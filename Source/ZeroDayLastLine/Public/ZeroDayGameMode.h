// Fill out your copyright notice in the Description page of Project Settings.

#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/GameModeBase.h" // Base class that defines the rules of the game.
#include "ZeroDayTypes.h" // Shared enums (EWallStatus, EResourceType, ...).
#include "ZeroDayLevelConfig.h" // The per-level difficulty table.
#include "ZeroDayGameMode.generated.h" // Unreal's generated reflection code (must be last include).

class AWallActor; // Forward declaration: the four city walls.
class ACityTaskActor; // Forward declaration: the power/food/medical stations.
class AThreatSpawnerActor; // Forward declaration: the things that send threats.
class AThreatPawn; // Forward declaration: the attackers.
class AResourcePickup; // Forward declaration: collectable resources.
class UCooldownComponent; // Forward declaration: the reusable timer.

// A short message shown by the HUD until ExpireTime (world seconds).
struct FZeroDayAlert // One on-screen message.
{
	FString Message; // The text to show.
	FColor Color = FColor::White; // The colour to show it in.
	float ExpireTime = 0.f; // World time (seconds) after which it disappears.
};

// A brief expanding ring the HUD draws at a world position to show something happened.
struct FZeroDayEffect // One expanding ring.
{
	FVector Location = FVector::ZeroVector; // Where in the world it happens.
	FLinearColor Color = FLinearColor::White; // Ring colour.
	float MaxRadius = 60.f; // How big the ring grows (world units).
	float StartTime = 0.f; // World time when it began.
	float Duration = 0.5f; // How long it lasts (seconds).
};

// What the player achieved on the current level, shown on the result screen.
struct FZeroDayLevelStats // The numbers shown on the result screen.
{
	int32 ThreatsStopped = 0; // Real threats shot down before they reached a wall.
	int32 FalsePositivesIdentified = 0; // False positives revealed by Validate.
	int32 WallsFixed = 0; // Breached walls the player patched.
	int32 SurvivorsLost = 0; // Survivors lost to stations that ran out.
	int32 SuppliesCollected = 0; // Resource pickups the player collected.
};

/**
 * Runs the three levels. It builds the city if the map has none, applies each level's
 * difficulty, starts surge waves, tracks the win/lose state, keeps the resource
 * inventory and survivors, and lets the player retry a level or move on to the next.
 */
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AZeroDayGameMode : public AGameModeBase // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AZeroDayGameMode(); // Sets the default pawn and HUD and creates the timers.

	// Which level's rules to apply (1-3). Changes as the player advances.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay") // Editable in the editor and Blueprints.
	int32 CurrentLevel = 1; // Starts on level 1.

	// When the map contains no walls, build a default city around the player at start.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout") // Editable in the editor and Blueprints.
	bool bAutoBuildLayout = true; // Build the city automatically when the map has none.

	// Distance from the city centre to the side walls (X) and to the front/back walls (Y).
	// Large enough that getting from a station to a wall is a real walk, not a step.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout") // Editable in the editor and Blueprints.
	FVector2D CityHalfExtents = FVector2D(1500.f, 950.f); // Half the city width (X) and depth (Y).

	// How far outside each wall its threat spawner sits.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout") // Editable in the editor and Blueprints.
	float SpawnerDistanceBeyondWall = 1300.f; // Spawners sit this far outside their wall. Increased from 750 because threats are now 4x bigger (radius 200), spread up to 400 units around the spawner, and the wall's trigger zone is 2x deeper: closer than this, new threats would appear already touching the wall and vanish.

	// How far each North/South wall is extended past the corner, so it overlaps the East/West
	// wall's end and the perimeter reads as one joined barrier instead of four separate strips
	// with gaps at the corners.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Layout") // Editable in the editor and Blueprints.
	float WallCornerOverlap = 180.f; // How far north/south walls run past the corners (doubled along with the wall thickness so the corners stay joined).

	// How many of a resource the player is carrying.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	int32 GetResource(EResourceType Type) const; // How many of a resource the player has.

	// True if the player has at least Amount of the resource.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	bool CanAfford(EResourceType Type, int32 Amount = 1) const; // True if enough is carried.

	// Adds resources (for example from a pickup).
	UFUNCTION(BlueprintCallable, Category = "ZeroDay") // Callable from Blueprints.
	void AddResource(EResourceType Type, int32 Amount = 1); // Adds to the inventory.

	// Spends resources if the player has enough. Returns false (and spends nothing) otherwise.
	UFUNCTION(BlueprintCallable, Category = "ZeroDay") // Callable from Blueprints.
	bool SpendResource(EResourceType Type, int32 Amount = 1); // All-or-nothing spend.

	// Survivors still alive, and how many there were at the start of the level.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	int32 GetSurvivors() const { return Survivors; } // Survivors still alive.

	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	int32 GetMaxSurvivors() const { return MaxSurvivors; } // Survivors at the start.

	// True once the current level has been won or lost.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	bool IsGameOver() const { return bGameEnded; } // True once the level has ended.

	// True if the level ended in a win.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	bool DidPlayerWin() const { return bPlayerWon; } // True if it ended in a win.

	// Seconds left to survive, for the HUD.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	float GetLevelTimeRemaining() const; // Seconds left on the level clock.

	// True after winning a level that has a next level.
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	bool CanAdvanceLevel() const; // Won and not the last level.

	// True after winning the last level (the whole game is complete).
	UFUNCTION(BlueprintPure, Category = "ZeroDay") // A read-only getter.
	bool IsFinalLevelComplete() const; // Won the last level.

	// Restarts the current level from scratch (bound to R).
	UFUNCTION(BlueprintCallable, Category = "ZeroDay") // Callable from Blueprints.
	void ReloadLevel(); // The R key.

	// Moves on to the next level after a win (bound to N).
	UFUNCTION(BlueprintCallable, Category = "ZeroDay") // Callable from Blueprints.
	void AdvanceToNextLevel(); // The N key.

	// Resets everything and begins the given level (1-3).
	void StartLevel(int32 Level); // Resets everything for the given level.

	// Shows a short status message to the player (scan results, wall warnings, etc.).
	void PostAlert(const FString& Message, const FColor& Color = FColor::Yellow); // Queues an on-screen message.

	// Shows a brief expanding ring at a world position (hits, breaches, scans).
	void PostEffect(const FVector& Location, const FLinearColor& Color, float MaxRadius = 60.f, float Duration = 0.5f); // Queues an expanding ring.

	// Called by a threat the moment the player's shots destroy it.
	void OnThreatKilled(const AThreatPawn* Threat); // Counts the kill and maybe drops loot.

	// Statistics recorded as the player plays, for the result screen.
	void RecordFalsePositivesIdentified(int32 Count) { Stats.FalsePositivesIdentified += Count; } // Validate found decoys.
	void RecordWallFixed() { ++Stats.WallsFixed; } // The player patched a wall.
	void RecordSupplyCollected() { ++Stats.SuppliesCollected; } // The player picked up a resource.
	const FZeroDayLevelStats& GetStats() const { return Stats; } // Read the totals.

	// The rules of the level currently being played.
	const FZeroDayLevelConfig& GetLevelConfig() const { return Config; } // The current level's rules.

	// Seconds since the current level began, for the HUD's start-of-level banner.
	float GetTimeSinceLevelStart() const; // Seconds since the level began.

	// Why the level ended (shown on the win/lose screen).
	const FString& GetEndMessage() const { return EndMessage; } // Why the level ended.

	const TArray<FZeroDayAlert>& GetAlerts() const { return Alerts; } // Current on-screen messages.
	const TArray<FZeroDayEffect>& GetEffects() const { return Effects; } // Rings currently animating.
	const TArray<TObjectPtr<AWallActor>>& GetWalls() const { return Walls; } // Every wall in the level.
	const TArray<TObjectPtr<ACityTaskActor>>& GetTasks() const { return Tasks; } // Every station in the level.
	const TArray<TObjectPtr<AThreatSpawnerActor>>& GetSpawners() const { return Spawners; } // Every spawner in the level.

protected:
	virtual void BeginPlay() override; // Finds or builds the city, then starts the level.

private:
	// Adds an actor to our lists and subscribes to its events (safe to call twice).
	void RegisterWall(AWallActor* Wall); // Tracks a wall.
	void RegisterTask(ACityTaskActor* Task); // Tracks a station.
	void RegisterSpawner(AThreatSpawnerActor* Spawner); // Tracks a spawner.

	// Creates four walls, four spawners and three stations around the player.
	void BuildDefaultLayout(); // Builds walls, spawners and stations.

	// Runs when a wall's status changes: turns it into an alert and an effect.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleWallStatusChanged(AWallActor* Wall, EWallStatus NewStatus); // Turns status changes into alerts.

	// Runs when a wall stayed breached too long: lose.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleWallLost(AWallActor* LostWall); // A lost wall loses the level.

	// Runs when a station went unattended too long: a survivor is lost.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleTaskFailed(ACityTaskActor* FailedTask); // A failed station costs a survivor.

	// Runs when the level timer ends with everything still standing: win.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleLevelSurvived(); // Surviving the clock wins.

	// Runs each time the surge timer fires (level 3): sends a critical wave at one wall.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleSurge(); // Sends a critical wave at one wall.

	// Runs each time the supply timer fires: a resource crate lands inside the city.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleSupplyDrop(); // Drops a crate inside the city.

	// Picks which resource to hand out next, favouring whichever the player has least of.
	EResourceType ChooseSupplyType() const; // Picks what the next crate holds.

	// Works out the middle of the city and how far supplies may land from it.
	void MeasureCity(); // Finds the city centre and drop radius.

	// Stops the timers and spawners and records the result for the HUD.
	void EndGame(bool bWon, const FString& Reason); // The single door out of a level.

	FZeroDayLevelConfig Config; // Rules of the current level.
	FZeroDayLevelStats Stats; // What the player has achieved this level.
	TArray<FZeroDayAlert> Alerts; // Messages currently shown by the HUD.
	TArray<FZeroDayEffect> Effects; // Rings currently animating.
	FString EndMessage; // Reason shown on the win/lose screen.
	int32 Inventory[ZeroDayResourceTypeCount] = { 0, 0, 0, 0, 0 }; // How many of each resource is carried.
	int32 Survivors = 5; // Survivors still alive.
	int32 MaxSurvivors = 5; // Survivors at the start of the level.
	bool bPlayerWon = false; // True if the level ended in a win.
	float LevelStartTime = 0.f; // World time when the current level began.

	// Survive-until timer: expiring while the game is still going means a win.
	UPROPERTY(VisibleAnywhere, Category = "ZeroDay") // Visible in the editor.
	TObjectPtr<UCooldownComponent> LevelTimer; // The level clock.

	// Looping timer that triggers a zero-day surge on levels that have them.
	UPROPERTY(VisibleAnywhere, Category = "ZeroDay") // Visible in the editor.
	TObjectPtr<UCooldownComponent> SurgeTimer; // The surge clock.

	// Looping timer that drops a resource crate inside the city.
	UPROPERTY(VisibleAnywhere, Category = "ZeroDay") // Visible in the editor.
	TObjectPtr<UCooldownComponent> SupplyTimer; // The supply drop clock.

	FVector CityCentre = FVector::ZeroVector; // Middle of the walls.
	float CitySupplyRadius = 300.f; // How far from the middle supply crates may land.

	// Everything below is found (or built) in the level at BeginPlay.
	UPROPERTY(Transient) // Not saved; rebuilt each run.
	TArray<TObjectPtr<AWallActor>> Walls; // Every wall.

	UPROPERTY(Transient) // Not saved; rebuilt each run.
	TArray<TObjectPtr<ACityTaskActor>> Tasks; // Every station.

	UPROPERTY(Transient) // Not saved; rebuilt each run.
	TArray<TObjectPtr<AThreatSpawnerActor>> Spawners; // Every spawner.

	// Latched so the game only ends once per level.
	bool bGameEnded = false; // True once the level has ended.
};
