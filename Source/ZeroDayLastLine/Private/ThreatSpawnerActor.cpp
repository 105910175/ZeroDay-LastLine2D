#include "ThreatSpawnerActor.h"
#include "ThreatPawn.h"
#include "WallActor.h"
#include "CooldownComponent.h"
#include "Components/SceneComponent.h"

// Just a transform in the level plus a timer; spawned threats appear at this actor's location.
AThreatSpawnerActor::AThreatSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Default to the shipped threat so a placed spawner works without Blueprint setup.
	ThreatClass = AThreatPawn::StaticClass();

	SpawnTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("SpawnTimer"));
}

// Starts spawning immediately; the game mode can pause it for inactive walls or when the game ends.
void AThreatSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	SpawnTimer->OnExpired.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnTick);
	SetSpawningEnabled(true);
}

// Starts a looping timer, or cancels it to stop spawning.
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

// Spawn one threat per tick, unless misconfigured, the wall is out of play, or the cap is reached.
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
		// Roll severity: Critical first, then Medium, otherwise Low.
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
		// Track when it dies so the concurrency cap stays accurate.
		NewThreat->OnDestroyed.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnedThreatDestroyed);
		LiveThreatCount++;
	}
}

// Fires whether the threat was shot or reached the wall.
void AThreatSpawnerActor::HandleSpawnedThreatDestroyed(AActor* DestroyedActor)
{
	LiveThreatCount = FMath::Max(0, LiveThreatCount - 1);
}
