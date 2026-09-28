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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AThreatPawn> ThreatClass;

	// The wall spawned threats walk towards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TObjectPtr<AWallActor> TargetWall;

	// Seconds between spawns.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnInterval = 4.f;

	// Spawning pauses while this many of its threats are still alive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 MaxConcurrentThreats = 5;

	// Chance (0-1) that a spawned threat is a harmless false positive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1"))
	float DecoyChance = 0.25f;

	// Chance (0-1) of a Critical / Medium threat. Whatever is left over is Low.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0", ClampMax = "1"))
	float MediumChance = 0.35f;

	// Multipliers the level applies to every threat this spawner creates.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float ThreatSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float ThreatHealthScale = 1.f;

	// Starts a looping timer, or cancels it to stop spawning.
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetSpawningEnabled(bool bEnabled);

	// Copies a level's difficulty settings onto this spawner.
	void ApplyLevelConfig(const FZeroDayLevelConfig& Config);

	// Spawns one threat with an exact severity and decoy flag. Returns true if it appeared.
	bool SpawnThreat(EThreatSeverity Severity, bool bDecoy);

	// Zero-day surge: spawns several real threats at once, ignoring the concurrency cap.
	void SpawnBurst(int32 Count, EThreatSeverity Severity);

protected:
	virtual void BeginPlay() override; // Hooks up the timer when the game starts.

	// Looping countdown that drives spawning.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UCooldownComponent> SpawnTimer;

private:
	// Runs each time the spawn timer expires.
	UFUNCTION()
	void HandleSpawnTick();

	// Runs when any spawned threat is destroyed.
	UFUNCTION()
	void HandleSpawnedThreatDestroyed(AActor* DestroyedActor);

	// How many threats from this spawner are currently alive.
	int32 LiveThreatCount = 0;
};
