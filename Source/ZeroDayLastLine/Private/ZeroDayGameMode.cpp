// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayGameMode.h"
#include "WallActor.h"
#include "CityTaskActor.h"
#include "ThreatSpawnerActor.h"
#include "CooldownComponent.h"
#include "ZeroDayTypes.h"
#include "ZeroDayCharacter.h"
#include "ZeroDayHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

AZeroDayGameMode::AZeroDayGameMode()
{
	DefaultPawnClass = AZeroDayCharacter::StaticClass();
	HUDClass = AZeroDayHUD::StaticClass();

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
			Wall->OnStatusUpdated.AddDynamic(this, &AZeroDayGameMode::HandleWallStatusChanged);
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

void AZeroDayGameMode::PostAlert(const FString& Message, const FColor& Color)
{
	constexpr int32 MaxAlerts = 6;
	constexpr float AlertLifetimeSeconds = 6.f;

	FZeroDayAlert Alert;
	Alert.Message = Message;
	Alert.Color = Color;
	Alert.ExpireTime = GetWorld()->GetTimeSeconds() + AlertLifetimeSeconds;
	Alerts.Add(Alert);

	if (Alerts.Num() > MaxAlerts)
	{
		Alerts.RemoveAt(0);
	}
}

float AZeroDayGameMode::GetLevelTimeRemaining() const
{
	return LevelTimer->GetTimeRemaining();
}

bool AZeroDayGameMode::CanLoadNextLevel() const
{
	return bGameEnded && bPlayerWon && !NextLevelName.IsNone();
}

void AZeroDayGameMode::ReloadLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void AZeroDayGameMode::LoadNextLevel()
{
	if (CanLoadNextLevel())
	{
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}

FString AZeroDayGameMode::GetObjectiveText() const
{
	switch (CurrentLevel)
	{
	case 1:
		return TEXT("Hold the north wall until the timer runs out. Scan (Q), validate (F), then shoot real threats.");
	case 2:
		return TEXT("Defend all four walls. Threats come from every side, so decide which exposure matters most.");
	default:
		return TEXT("Defend every wall and keep power, food and medical care running (E at each station).");
	}
}

void AZeroDayGameMode::HandleWallStatusChanged(AWallActor* Wall, EWallStatus NewStatus)
{
	if (!Wall || bGameEnded)
	{
		return;
	}

	const TCHAR* Side = WallSideToString(Wall->WallSide);
	switch (NewStatus)
	{
	case EWallStatus::Threatened:
		PostAlert(FString::Printf(TEXT("%s wall under attack (integrity %.0f%%)"), Side, Wall->GetHealthPercent() * 100.f), FColor::Orange);
		break;
	case EWallStatus::Breached:
		PostAlert(FString::Printf(TEXT("%s wall BREACHED: fix it (E) or clear the threats!"), Side), FColor::Red);
		break;
	default:
		PostAlert(FString::Printf(TEXT("%s wall secured"), Side), FColor::Green);
		break;
	}
}

void AZeroDayGameMode::HandleWallLost(AWallActor* LostWall)
{
	EndGame(false, FString::Printf(TEXT("The %s wall fell and the zero-day was weaponised."), LostWall ? WallSideToString(LostWall->WallSide) : TEXT("outer")));
}

void AZeroDayGameMode::HandleTaskFailed(ACityTaskActor* FailedTask)
{
	EndGame(false, FString::Printf(TEXT("%s failed while you were busy elsewhere."), FailedTask ? TaskKindToString(FailedTask->TaskKind) : TEXT("A city task")));
}

void AZeroDayGameMode::HandleLevelSurvived()
{
	EndGame(true, TEXT("Zero-day contained. The city survives."));
}

void AZeroDayGameMode::EndGame(bool bWon, const FString& Reason)
{
	if (bGameEnded)
	{
		return;
	}
	bGameEnded = true;
	bPlayerWon = bWon;
	EndMessage = Reason;

	LevelTimer->CancelCooldown();
	for (AThreatSpawnerActor* Spawner : Spawners)
	{
		if (Spawner)
		{
			Spawner->SetSpawningEnabled(false);
		}
	}
}
