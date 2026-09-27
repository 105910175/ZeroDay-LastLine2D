#include "ThreatSpawnerActor.h"
#include "ThreatPawn.h"
#include "WallActor.h"
#include "CooldownComponent.h"
#include "Components/SceneComponent.h"

AThreatSpawnerActor::AThreatSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Default to the shipped threat so a placed spawner works without Blueprint setup.
	ThreatClass = AThreatPawn::StaticClass();

	SpawnTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("SpawnTimer"));
}

void AThreatSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	SpawnTimer->OnExpired.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnTick);
	SetSpawningEnabled(true);
}

void AThreatSpawnerActor::SetSpawningEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		SpawnTimer->StartCooldown(SpawnInterval, true);
	}
	else
	{
		SpawnTimer->CancelCooldown();
	}
}

void AThreatSpawnerActor::HandleSpawnTick()
{
	if (!ThreatClass || !TargetWall || !TargetWall->bIsActiveThisLevel)
	{
		return;
	}

	if (LiveThreatCount >= MaxConcurrentThreats)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AThreatPawn* NewThreat = GetWorld()->SpawnActor<AThreatPawn>(ThreatClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	if (NewThreat)
	{
		const float SeverityRoll = FMath::FRand();
		EThreatSeverity Severity = EThreatSeverity::Low;
		if (SeverityRoll < CriticalChance)
		{
			Severity = EThreatSeverity::Critical;
		}
		else if (SeverityRoll < CriticalChance + MediumChance)
		{
			Severity = EThreatSeverity::Medium;
		}

		NewThreat->InitialiseThreat(TargetWall, Severity, FMath::FRand() < DecoyChance);
		NewThreat->OnDestroyed.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnedThreatDestroyed);
		LiveThreatCount++;
	}
}

void AThreatSpawnerActor::HandleSpawnedThreatDestroyed(AActor* DestroyedActor)
{
	LiveThreatCount = FMath::Max(0, LiveThreatCount - 1);
}
