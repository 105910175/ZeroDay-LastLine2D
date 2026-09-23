// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZeroDayGameMode.generated.h"

class AWallActor;
class ACityTaskActor;
class AThreatSpawnerActor;
class UCooldownComponent;

/**
 * Configures which walls/tasks are active for CurrentLevel (1-3, per the
 * planning doc's progression table) and ends the game the moment any wall is
 * lost or any city task fails critically.
 */
UCLASS()
class ZERODAYLASTLINE_API AZeroDayGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZeroDayGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay")
	int32 CurrentLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay")
	float LevelDurationSeconds = 90.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZeroDay")
	int32 Resources = 12;

	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	bool SpendResource(int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool IsGameOver() const { return bGameEnded; }

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureLevel();

	UFUNCTION()
	void HandleWallLost(AWallActor* LostWall);

	UFUNCTION()
	void HandleTaskFailed(ACityTaskActor* FailedTask);

	UFUNCTION()
	void HandleLevelSurvived();

	void EndGame(bool bWon);

	UPROPERTY(VisibleAnywhere, Category = "ZeroDay")
	TObjectPtr<UCooldownComponent> LevelTimer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWallActor>> Walls;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityTaskActor>> Tasks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AThreatSpawnerActor>> Spawners;

	bool bGameEnded = false;
};
