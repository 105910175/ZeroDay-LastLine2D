#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayTypes.h" // EThreatSeverity.
#include "ThreatSpawnerActor.generated.h" // Unreal's generated reflection code (must be last include).

class AThreatPawn; // Forward declaration: what gets spawned.
class AWallActor; // Forward declaration: where spawned threats walk to.
class UCooldownComponent; // Forward declaration: the spawn timer.
struct FZeroDayLevelConfig; // Forward declaration: the level rules this spawner obeys.

// One of these sits outside each wall and assigns its TargetWall. It uses the same
// UCooldownComponent as the walls and stations, looped, to drive its spawn interval.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AThreatSpawnerActor : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AThreatSpawnerActor(); // Builds the root and timer.

	// Which threat class to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	TSubclassOf<AThreatPawn> ThreatClass; // Defaults to AThreatPawn.

	// The wall spawned threats walk towards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	TObjectPtr<AWallActor> TargetWall; // Set when the city is built.

	// Seconds between spawns.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	float SpawnInterval = 4.f; // The level config overrides this.

	// Spawning pauses while this many of its threats are still alive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	int32 MaxConcurrentThreats = 5; // The level config overrides this.

	// Chance (0-1) that a spawned threat is a harmless false positive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1")) // Editable, limited to 0-1.
	float DecoyChance = 0.25f; // The level config overrides this.

	// Chance (0-1) of a Critical / Medium threat. Whatever is left over is Low.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1")) // Editable, limited to 0-1.
	float CriticalChance = 0.15f; // The level config overrides this.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1")) // Editable, limited to 0-1.
	float MediumChance = 0.35f; // The level config overrides this.

	// Multipliers the level applies to every threat this spawner creates.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	float ThreatSpeedScale = 1.f; // Applied to each threat's speed.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner") // Editable in the editor and Blueprints.
	float ThreatHealthScale = 1.f; // Applied to each threat's health.

	// Starts a looping timer, or cancels it to stop spawning.
	UFUNCTION(BlueprintCallable, Category = "Spawner") // Callable from Blueprints.
	void SetSpawningEnabled(bool bEnabled); // On at level start, off when the game ends.

	// Copies a level's difficulty settings onto this spawner.
	void ApplyLevelConfig(const FZeroDayLevelConfig& Config); // Called at the start of every level.

	// Spawns one threat with an exact severity and decoy flag. Returns true if it appeared.
	bool SpawnThreat(EThreatSeverity Severity, bool bDecoy); // Used by the timer, surges and tests.

	// Zero-day surge: spawns several real threats at once, ignoring the concurrency cap.
	void SpawnBurst(int32 Count, EThreatSeverity Severity); // Used by level 3 surges.

protected:
	virtual void BeginPlay() override; // Hooks up the timer when the game starts.

	// Looping countdown that drives spawning.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner") // Visible in the editor.
	TObjectPtr<UCooldownComponent> SpawnTimer; // Fires every SpawnInterval seconds.

private:
	// Runs each time the spawn timer expires.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleSpawnTick(); // Rolls severity and decoy status, then spawns.

	// Runs when any spawned threat is destroyed.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleSpawnedThreatDestroyed(AActor* DestroyedActor); // Keeps the live count accurate.

	// How many threats from this spawner are currently alive.
	int32 LiveThreatCount = 0; // Compared against MaxConcurrentThreats.
};
