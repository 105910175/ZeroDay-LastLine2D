#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThreatSpawnerActor.generated.h"

class AThreatPawn;
class AWallActor;
class UCooldownComponent;

// Place one of these near each wall in the level and assign its TargetWall.
// Uses the same UCooldownComponent as AWallActor/ACityTaskActor, looped, to
// drive its spawn interval.
UCLASS()
class ZERODAYLASTLINE_API AThreatSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	AThreatSpawnerActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TSubclassOf<AThreatPawn> ThreatClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TObjectPtr<AWallActor> TargetWall;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnInterval = 4.f;

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

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetSpawningEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UCooldownComponent> SpawnTimer;

private:
	UFUNCTION()
	void HandleSpawnTick();

	UFUNCTION()
	void HandleSpawnedThreatDestroyed(AActor* DestroyedActor);

	int32 LiveThreatCount = 0;
};
