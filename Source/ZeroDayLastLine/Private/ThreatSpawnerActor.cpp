#include "ThreatSpawnerActor.h"
#include "ThreatPawn.h"
#include "WallActor.h"
#include "CooldownComponent.h"
#include "Components/SceneComponent.h"

AThreatSpawnerActor::AThreatSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

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
		NewThreat->TargetWall = TargetWall;
		NewThreat->OnDestroyed.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnedThreatDestroyed);
		LiveThreatCount++;
	}
}

void AThreatSpawnerActor::HandleSpawnedThreatDestroyed(AActor* DestroyedActor)
{
	LiveThreatCount = FMath::Max(0, LiveThreatCount - 1);
}
