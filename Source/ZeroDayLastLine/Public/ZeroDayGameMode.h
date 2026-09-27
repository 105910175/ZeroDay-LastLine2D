// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZeroDayTypes.h"
#include "ZeroDayGameMode.generated.h"

class AWallActor;
class ACityTaskActor;
class AThreatSpawnerActor;
class UCooldownComponent;

// A short message shown by the HUD until ExpireTime (world seconds).
struct FZeroDayAlert
{
	FString Message;
	FColor Color = FColor::White;
	float ExpireTime = 0.f;
};

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

	// Map to load when the player wins and presses N. Leave empty for the last level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay")
	FName NextLevelName;

	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool DidPlayerWin() const { return bPlayerWon; }

	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	float GetLevelTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool CanLoadNextLevel() const;

	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	void ReloadLevel();

	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	void LoadNextLevel();

	// Shows a short status message to the player (scan results, wall warnings, etc.).
	void PostAlert(const FString& Message, const FColor& Color = FColor::Yellow);

	// What the player should be trying to do on the current level.
	FString GetObjectiveText() const;

	// Why the level ended (shown on the win/lose screen).
	const FString& GetEndMessage() const { return EndMessage; }

	const TArray<FZeroDayAlert>& GetAlerts() const { return Alerts; }
	const TArray<TObjectPtr<AWallActor>>& GetWalls() const { return Walls; }
	const TArray<TObjectPtr<ACityTaskActor>>& GetTasks() const { return Tasks; }

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureLevel();

	UFUNCTION()
	void HandleWallLost(AWallActor* LostWall);

	UFUNCTION()
	void HandleWallStatusChanged(AWallActor* Wall, EWallStatus NewStatus);

	UFUNCTION()
	void HandleTaskFailed(ACityTaskActor* FailedTask);

	UFUNCTION()
	void HandleLevelSurvived();

	void EndGame(bool bWon, const FString& Reason);

	TArray<FZeroDayAlert> Alerts;
	FString EndMessage;
	bool bPlayerWon = false;

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
