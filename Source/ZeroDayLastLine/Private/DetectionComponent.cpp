#include "DetectionComponent.h" // This class's own declaration.
#include "WallActor.h" // The walls being scanned and validated.
#include "ZeroDayGameMode.h" // Where resources are spent and alerts are shown.
#include "ZeroDayTypes.h" // Shared enums and name helpers.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "ZeroDayCharacter.h" // The player, whose audio component plays the scan sounds.
#include "Audio/ZeroDayAudioComponent.h" // The sound effect player.

// No tick: everything happens in response to player input.
UDetectionComponent::UDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Never needs a per-frame update.
}

// Seconds until Insta Scan can be used again, for the HUD.
float UDetectionComponent::GetScanCooldownRemaining() const
{
	if (const UWorld* World = GetWorld()) // We need the world's clock.
	{
		return FMath::Max(0.f, NextScanTime - World->GetTimeSeconds()); // Time left, never negative.
	}
	return 0.f; // No world means no cooldown.
}

// Insta Scan (Q): flags every inbound threat on every active wall. Fast and free, but it can't tell real threats from false positives.
void UDetectionComponent::PerformScan()
{
	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)); // Find our game mode.
	if (!GameMode || GameMode->IsGameOver()) // Nothing to scan with no game or once the level has ended.
	{
		return; // Ignore the key press.
	}

	const float CooldownLeft = GetScanCooldownRemaining(); // Seconds before the next scan is allowed.
	if (CooldownLeft > 0.f) // Still recharging?
	{
		GameMode->PostAlert(FString::Printf(TEXT("Insta Scan recharging (%.0fs)"), CooldownLeft), FColor::Silver); // Tell the player to wait.
		return; // Do nothing else.
	}
	// Start the cooldown as soon as a scan is used.
	NextScanTime = GetWorld()->GetTimeSeconds() + ScanCooldownSeconds; // The next scan is allowed after the cooldown.

	if (AZeroDayCharacter* Character = Cast<AZeroDayCharacter>(GetOwner())) // The player who owns this component.
	{
		if (UZeroDayAudioComponent* AudioComponent = // The sound player.
			Character->GetAudioComponent())
		{
			AudioComponent->PlayScan(); // Play the scan sound.
		}
	}

	if (GetOwner()) // If we know where the player is...
	{
		GameMode->PostEffect( // ...send a wide cyan scan pulse out from them.
			GetOwner()->GetActorLocation(), // From the player.
			FLinearColor(0.3f, 0.8f, 1.f), // Cyan.
			900.f, // A wide ring.
			0.7f // Fairly quick.
		);
	}

	int32 TotalContacts = 0; // How many threats the scan finds in total.
	for (AWallActor* Wall : GameMode->GetWalls()) // Look at every wall.
	{
		if (!Wall || !Wall->bIsActiveThisLevel) // Skip missing walls and walls not in play.
		{
			continue; // Next wall.
		}

		const int32 Contacts = Wall->MarkInboundDetected(); // Flag every threat heading for this wall.
		if (Contacts > 0) // Did we find any?
		{
			TotalContacts += Contacts; // Add to the running total.
			GameMode->PostAlert(FString::Printf(TEXT("Insta Scan: exposure detected on %s wall (%d contact%s)"), // Report this wall.
				WallSideToString(Wall->WallSide), Contacts, Contacts == 1 ? TEXT("") : TEXT("s")), FColor(255, 105, 90)); // Name, count and plural.
		}
	}

	if (TotalContacts == 0) // Nothing anywhere?
	{
		GameMode->PostAlert(TEXT("Insta Scan: no exposure detected"), FColor::Green); // Reassure the player.
	}
}

// Validate (F): checks the nearest scanned wall in range. Costs a resource, reveals what each threat really is.
void UDetectionComponent::PerformValidate()
{
	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)); // Find our game mode.
	if (!GameMode || GameMode->IsGameOver() || !GetOwner()) // Need a game, a live level and a player.
	{
		return; // Ignore the key press.
	}

	// Pick the nearest wall that has been scanned and is within ValidateRange.
	AWallActor* Target = nullptr; // The chosen wall, if any.
	float BestDistance = ValidateRange; // Only walls closer than this qualify.
	for (AWallActor* Wall : GameMode->GetWalls()) // Look at every wall.
	{
		if (!Wall || !Wall->bIsActiveThisLevel || !Wall->IsExposureDetected()) // Must exist, be in play and have been scanned.
		{
			continue; // Next wall.
		}

		const float Distance = FVector::Dist(Wall->GetActorLocation(), GetOwner()->GetActorLocation()); // How far the player is from it.
		if (Distance <= BestDistance) // Closer than the best so far?
		{
			BestDistance = Distance; // New best distance.
			Target = Wall; // New best wall.
		}
	}

	if (!Target) // No wall qualified.
	{
		GameMode->PostAlert(TEXT("Validate: no scanned wall in range. Run Insta Scan and move closer."), FColor::Silver); // Explain what to do.
		return; // Nothing to validate.
	}

	if (Target->GetUnvalidatedDetectedCount() == 0) // Everything there was already checked.
	{
		GameMode->PostAlert(FString::Printf(TEXT("Validate: %s wall already checked"), WallSideToString(Target->WallSide)), FColor::Silver); // Don't charge twice.
		return; // Nothing new to learn.
	}

	// Pay only after every other check has passed.
	if (!GameMode->SpendResource(EResourceType::Intel, ValidateCost)) // Try to pay the Intel cost.
	{
		GameMode->PostAlert(TEXT("Validate needs Intel. Collect some first."), FColor::Orange); // Say why it failed.
		return; // Could not afford it.
	}

	int32 Real = 0; // How many real threats were found.
	int32 FalsePositives = 0; // How many false positives were found.
	EThreatSeverity Highest = EThreatSeverity::Low; // The worst real threat found.
	Target->ValidateInbound(Real, FalsePositives, Highest); // Reveal the truth about every scanned threat on that wall.

	if (AZeroDayCharacter* Character = Cast<AZeroDayCharacter>(GetOwner())) // The player who owns this component.
	{
		if (UZeroDayAudioComponent* AudioComponent = // The sound player.
			Character->GetAudioComponent())
		{
			AudioComponent->PlayValidate(Target->GetActorLocation()); // Play the validate sound from the wall.
		}
	}

	GameMode->RecordFalsePositivesIdentified(FalsePositives); // Count the decoys we exposed for the result screen.
	GameMode->PostEffect(Target->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.3f), 220.f, 0.6f); // Gold ring on the wall that was validated.

	FString Result = FString::Printf(TEXT("Validate (%s wall): "), WallSideToString(Target->WallSide)); // Start the result message.
	if (Real > 0) // Were any of them real?
	{
		Result += FString::Printf(TEXT("%d real, highest severity %s"), Real, SeverityToString(Highest)); // Say how many and how bad.
	}
	else
	{
		Result += TEXT("no real threats"); // All of them were false positives.
	}
	if (FalsePositives > 0) // Were any of them harmless?
	{
		Result += FString::Printf(TEXT(", %d false positive%s"), FalsePositives, FalsePositives == 1 ? TEXT("") : TEXT("s")); // Add the false positive count.
	}
	GameMode->PostAlert(Result, Real > 0 ? FColor::Orange : FColor::Green); // Orange if there is danger, green if it is all clear.
}
