// Fill out your copyright notice in the Description page of Project Settings.

#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/GameModeBase.h" // Base class that defines the rules of the game.
#include "ZeroDayTypes.h" // Shared enums (EWallStatus, ...).
#include "ZeroDayLevelConfig.h" // The per-level difficulty table.
#include "ZeroDayGameMode.generated.h" // Unreal's generated reflection code (must be last include).

class AWallActor; // Forward declaration: the four city walls.
class ACityTaskActor; // Forward declaration: the power/food/medical stations.
class AThreatSpawnerActor; // Forward declaration: the things that send threats.
class UCooldownComponent; // Forward declaration: the reusable timer.

// A short message shown by the HUD until ExpireTime (world seconds).
struct FZeroDayAlert
{
	FString Message; // The text to show.
	FColor Color = FColor::White; // The colour to show it in.
	float ExpireTime = 0.f; // World time (seconds) after which it disappears.
};

/**
 * Runs the three levels. It builds the city if the map has none, applies each level's
 * difficulty, starts surge waves, tracks the win/lose state, and lets the player retry
 * a level or move on to the next one.
 */
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AZeroDayGameMode : public AGameModeBase // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AZeroDayGameMode(); // Sets the default pawn and HUD and creates the timers.

	// Which level's rules to apply (1-3). Changes as the player advances.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay")
	int32 CurrentLevel = 1;

	// Shared resource pool, spent by Validate and by attending city stations.
	UPROPERTY(BlueprintReadOnly, Category = "ZeroDay")
	int32 Resources = 6;

	// When the map contains no walls, build a default city around the player at start.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout")
	bool bAutoBuildLayout = true;

	// Distance from the city centre to the side walls (X) and to the front/back walls (Y).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout")
	FVector2D CityHalfExtents = FVector2D(1000.f, 650.f);

	// How far outside each wall its threat spawner sits.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout")
	float SpawnerDistanceBeyondWall = 600.f;

	// Spends resources if the player has enough. Returns false (and spends nothing) otherwise.
	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	bool SpendResource(int32 Amount = 1);

	// True once the current level has been won or lost.
	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool IsGameOver() const { return bGameEnded; }

	// True if the level ended in a win.
	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool DidPlayerWin() const { return bPlayerWon; }

	// Seconds left to survive, for the HUD.
	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	float GetLevelTimeRemaining() const;

	// True after winning a level that has a next level.
	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool CanAdvanceLevel() const;

	// True after winning the last level (the whole game is complete).
	UFUNCTION(BlueprintPure, Category = "ZeroDay")
	bool IsFinalLevelComplete() const;

	// Restarts the current level from scratch (bound to R).
	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	void ReloadLevel();

	// Moves on to the next level after a win (bound to N).
	UFUNCTION(BlueprintCallable, Category = "ZeroDay")
	void AdvanceToNextLevel();

	// Resets everything and begins the given level (1-3).
	void StartLevel(int32 Level);

	// Shows a short status message to the player (scan results, wall warnings, etc.).
	void PostAlert(const FString& Message, const FColor& Color = FColor::Yellow);

	// The rules of the level currently being played.
	const FZeroDayLevelConfig& GetLevelConfig() const { return Config; }

	// Seconds since the current level began, for the HUD's start-of-level banner.
	float GetTimeSinceLevelStart() const;

	// Why the level ended (shown on the win/lose screen).
	const FString& GetEndMessage() const { return EndMessage; }

	const TArray<FZeroDayAlert>& GetAlerts() const { return Alerts; } // Current on-screen messages.
	const TArray<TObjectPtr<AWallActor>>& GetWalls() const { return Walls; } // Every wall in the level.
	const TArray<TObjectPtr<ACityTaskActor>>& GetTasks() const { return Tasks; } // Every station in the level.
	const TArray<TObjectPtr<AThreatSpawnerActor>>& GetSpawners() const { return Spawners; } // Every spawner in the level.

protected:
	virtual void BeginPlay() override; // Finds or builds the city, then starts the level.

private:
	// Adds an actor to our lists and subscribes to its events (safe to call twice).
	void RegisterWall(AWallActor* Wall);
	void RegisterTask(ACityTaskActor* Task);
	void RegisterSpawner(AThreatSpawnerActor* Spawner);

	// Creates four walls, four spawners and three stations around the player.
	void BuildDefaultLayout();

	// Runs when a wall's status changes: turns it into an alert.
	UFUNCTION()
	void HandleWallStatusChanged(AWallActor* Wall, EWallStatus NewStatus);

	// Runs when a wall stayed breached too long: lose.
	UFUNCTION()
	void HandleWallLost(AWallActor* LostWall);

	// Runs when a station went unattended too long: lose.
	UFUNCTION()
	void HandleTaskFailed(ACityTaskActor* FailedTask);

	// Runs when the level timer ends with everything still standing: win.
	UFUNCTION()
	void HandleLevelSurvived();

	// Runs each time the surge timer fires (level 3): sends a critical wave at one wall.
	UFUNCTION()
	void HandleSurge();

	// Stops the timers and spawners and records the result for the HUD.
	void EndGame(bool bWon, const FString& Reason);

	FZeroDayLevelConfig Config; // Rules of the current level.
	TArray<FZeroDayAlert> Alerts; // Messages currently shown by the HUD.
	FString EndMessage; // Reason shown on the win/lose screen.
	bool bPlayerWon = false; // True if the level ended in a win.
	float LevelStartTime = 0.f; // World time when the current level began.

	// Survive-until timer: expiring while the game is still going means a win.
	UPROPERTY(VisibleAnywhere, Category = "ZeroDay")
	TObjectPtr<UCooldownComponent> LevelTimer;

	// Looping timer that triggers a zero-day surge on levels that have them.
	UPROPERTY(VisibleAnywhere, Category = "ZeroDay")
	TObjectPtr<UCooldownComponent> SurgeTimer;

	// Everything below is found (or built) in the level at BeginPlay.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AWallActor>> Walls;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityTaskActor>> Tasks;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AThreatSpawnerActor>> Spawners;

	// Latched so the game only ends once per level.
	bool bGameEnded = false;
};
