#include "DetectionComponent.h"
#include "WallActor.h"
#include "ZeroDayGameMode.h"
#include "ZeroDayTypes.h"
#include "Kismet/GameplayStatics.h"

UDetectionComponent::UDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UDetectionComponent::GetScanCooldownRemaining() const
{
	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.f, NextScanTime - World->GetTimeSeconds());
	}
	return 0.f;
}

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
	NextScanTime = GetWorld()->GetTimeSeconds() + ScanCooldownSeconds;

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
				WallSideToString(Wall->WallSide), Contacts, Contacts == 1 ? TEXT("") : TEXT("s")), FColor::Red);
		}
	}

	if (TotalContacts == 0)
	{
		GameMode->PostAlert(TEXT("Insta Scan: no exposure detected"), FColor::Green);
	}
}

void UDetectionComponent::PerformValidate()
{
	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GameMode || GameMode->IsGameOver() || !GetOwner())
	{
		return;
	}

	// Validate the nearest wall that has been scanned and is in range.
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

	if (!GameMode->SpendResource(ValidateCost))
	{
		GameMode->PostAlert(TEXT("Validate: not enough resources"), FColor::Orange);
		return;
	}

	int32 Real = 0;
	int32 FalsePositives = 0;
	EThreatSeverity Highest = EThreatSeverity::Low;
	Target->ValidateInbound(Real, FalsePositives, Highest);

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
