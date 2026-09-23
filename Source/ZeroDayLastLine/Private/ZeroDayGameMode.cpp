// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayGameMode.h"
#include "WallActor.h"
#include "CityTaskActor.h"
#include "ThreatSpawnerActor.h"
#include "CooldownComponent.h"
#include "ZeroDayTypes.h"
#include "ZeroDayCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AZeroDayGameMode::AZeroDayGameMode()
{
	DefaultPawnClass = AZeroDayCharacter::StaticClass();

	LevelTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("LevelTimer"));
}

void AZeroDayGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundWalls;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWallActor::StaticClass(), FoundWalls);
	for (AActor* Actor : FoundWalls)
	{
		if (AWallActor* Wall = Cast<AWallActor>(Actor))
		{
			Walls.Add(Wall);
			Wall->OnWallLost.AddDynamic(this, &AZeroDayGameMode::HandleWallLost);
		}
	}

	TArray<AActor*> FoundTasks;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACityTaskActor::StaticClass(), FoundTasks);
	for (AActor* Actor : FoundTasks)
	{
		if (ACityTaskActor* Task = Cast<ACityTaskActor>(Actor))
		{
			Tasks.Add(Task);
			Task->OnTaskFailed.AddDynamic(this, &AZeroDayGameMode::HandleTaskFailed);
		}
	}

	TArray<AActor*> FoundSpawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThreatSpawnerActor::StaticClass(), FoundSpawners);
	for (AActor* Actor : FoundSpawners)
	{
		if (AThreatSpawnerActor* Spawner = Cast<AThreatSpawnerActor>(Actor))
		{
			Spawners.Add(Spawner);
		}
	}

	ConfigureLevel();

	LevelTimer->OnExpired.AddDynamic(this, &AZeroDayGameMode::HandleLevelSurvived);
	LevelTimer->StartCooldown(LevelDurationSeconds, false);
}

void AZeroDayGameMode::ConfigureLevel()
{
	// Level 1: North wall only, no city tasks. Level 2: all walls, no tasks.
	// Level 3: all walls + all tasks running concurrently. See the planning doc's
	// level progression table.
	for (AWallActor* Wall : Walls)
	{
		if (!Wall)
		{
			continue;
		}
		const bool bActive = (CurrentLevel == 1) ? (Wall->WallSide == EWallSide::North) : true;
		Wall->SetActiveThisLevel(bActive);
	}

	for (ACityTaskActor* Task : Tasks)
	{
		if (Task)
		{
			Task->SetActiveThisLevel(CurrentLevel >= 3);
		}
	}

	for (AThreatSpawnerActor* Spawner : Spawners)
	{
		if (Spawner)
		{
			Spawner->SetSpawningEnabled(true);
		}
	}
}

bool AZeroDayGameMode::SpendResource(int32 Amount)
{
	if (Resources < Amount)
	{
		return false;
	}
	Resources -= Amount;
	return true;
}

void AZeroDayGameMode::HandleWallLost(AWallActor* LostWall)
{
	EndGame(false);
}

void AZeroDayGameMode::HandleTaskFailed(ACityTaskActor* FailedTask)
{
	EndGame(false);
}

void AZeroDayGameMode::HandleLevelSurvived()
{
	EndGame(true);
}

void AZeroDayGameMode::EndGame(bool bWon)
{
	if (bGameEnded)
	{
		return;
	}
	bGameEnded = true;

	LevelTimer->CancelCooldown();
	for (AThreatSpawnerActor* Spawner : Spawners)
	{
		if (Spawner)
		{
			Spawner->SetSpawningEnabled(false);
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, bWon ? FColor::Green : FColor::Red,
			bWon ? TEXT("Zero-day contained. City survives.") : TEXT("A wall has fallen. Zero-day weaponised."));
	}
}
