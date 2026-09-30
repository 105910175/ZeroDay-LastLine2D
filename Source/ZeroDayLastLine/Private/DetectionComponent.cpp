#include "DetectionComponent.h"
#include "WallActor.h"
#include "ZeroDayGameMode.h"
#include "ZeroDayTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ZeroDayCharacter.h"
#include "Audio/ZeroDayAudioComponent.h"

// No tick: everything happens in response to player input.
UDetectionComponent::UDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Seconds until Insta Scan can be used again, for the HUD.
float UDetectionComponent::GetScanCooldownRemaining() const
{
	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.f, NextScanTime - World->GetTimeSeconds());
	}
	return 0.f;
}

// Insta Scan (Q): flags every inbound threat on every active wall. Fast and free, but it can't tell real threats from false positives.
void UDetectionComponent::PerformScan()
{
	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode || GameMode->IsGameOver())
	{
		return;
	}

	const float CooldownLeft = GetScanCooldownRemaining();
	if (CooldownLeft > 0.f)
	{
		GameMode->PostAlert(FString::Printf(TEXT("Insta Scan recharging (%.0fs)"), CooldownLeft), FColor::Silver);
		return;
	}
	// Start the cooldown as soon as a scan is used.
    NextScanTime = GetWorld()->GetTimeSeconds() + ScanCooldownSeconds;

    if (AZeroDayCharacter* Character = Cast<AZeroDayCharacter>(GetOwner()))
    {
        if (UZeroDayAudioComponent* AudioComponent =
            Character->GetAudioComponent())
        {
            AudioComponent->PlayScan();
        }
    }

    if (GetOwner())
    {
        GameMode->PostEffect(
            GetOwner()->GetActorLocation(),
            FLinearColor(0.3f, 0.8f, 1.f),
            900.f,
            0.7f
        );
    }

	int32 TotalContacts = 0;
	for (AWallActor* Wall : GameMode->GetWalls())
	{
		if (!Wall || !Wall->bIsActiveThisLevel)
		{
			continue;
		}

		const int32 Contacts = Wall->MarkInboundDetected();
		if (Contacts > 0)
		{
			TotalContacts += Contacts;
			GameMode->PostAlert(FString::Printf(TEXT("Insta Scan: exposure detected on %s wall (%d contact%s)"),
				WallSideToString(Wall->WallSide), Contacts, Contacts == 1 ? TEXT("") : TEXT("s")), FColor(255, 105, 90));
		}
	}

	if (TotalContacts == 0)
	{
		GameMode->PostAlert(TEXT("Insta Scan: no exposure detected"), FColor::Green);
	}
}

// Validate (F): checks the nearest scanned wall in range. Costs a resource, reveals what each threat really is.
void UDetectionComponent::PerformValidate()
{
	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode || GameMode->IsGameOver() || !GetOwner())
	{
		return;
	}

	// Pick the nearest wall that has been scanned and is within ValidateRange.
	AWallActor* Target = nullptr;
	float BestDistance = ValidateRange;
	for (AWallActor* Wall : GameMode->GetWalls())
	{
		if (!Wall || !Wall->bIsActiveThisLevel || !Wall->IsExposureDetected())
		{
			continue;
		}

		const float Distance = FVector::Dist(Wall->GetActorLocation(), GetOwner()->GetActorLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Target = Wall;
		}
	}

	if (!Target)
	{
		GameMode->PostAlert(TEXT("Validate: no scanned wall in range. Run Insta Scan and move closer."), FColor::Silver);
		return;
	}

	if (Target->GetUnvalidatedDetectedCount() == 0)
	{
		GameMode->PostAlert(FString::Printf(TEXT("Validate: %s wall already checked"), WallSideToString(Target->WallSide)), FColor::Silver);
		return;
	}

	// Pay only after every other check has passed.
	if (!GameMode->SpendResource(EResourceType::Intel, ValidateCost))
	{
		GameMode->PostAlert(TEXT("Validate needs Intel. Collect some first."), FColor::Orange);
		return;
	}

	int32 Real = 0;
	int32 FalsePositives = 0;
	EThreatSeverity Highest = EThreatSeverity::Low;
	Target->ValidateInbound(Real, FalsePositives, Highest);
	GameMode->RecordFalsePositivesIdentified(FalsePositives); // Count the decoys we exposed for the result screen.
	GameMode->PostEffect(Target->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.3f), 220.f, 0.6f); // Gold ring on the wall that was validated.

	FString Result = FString::Printf(TEXT("Validate (%s wall): "), WallSideToString(Target->WallSide));
	if (Real > 0)
	{
		Result += FString::Printf(TEXT("%d real, highest severity %s"), Real, SeverityToString(Highest));
	}
	else
	{
		Result += TEXT("no real threats");
	}
	if (FalsePositives > 0)
	{
		Result += FString::Printf(TEXT(", %d false positive%s"), FalsePositives, FalsePositives == 1 ? TEXT("") : TEXT("s"));
	}
	GameMode->PostAlert(Result, Real > 0 ? FColor::Orange : FColor::Green);
}
