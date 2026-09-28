#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayTypes.h" // ECityTaskKind.
#include "CityTaskActor.generated.h" // Unreal's generated reflection code (must be last include).

class UCityTaskComponent; // Forward declaration: the decaying meter.
class UCooldownComponent; // Forward declaration: the grace timer.
class UStaticMeshComponent; // Forward declaration: the visible station.
class ACityTaskActor; // Forward declaration so the delegate below can name this class.

// Fired when a station runs out its grace period while still critical.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityTaskFailed, ACityTaskActor*, FailedTask);

// Power / food / medic tasks inside the walls. This is the other half of Validate &
// Prioritise: the player has to choose between running to a wall or attending a task
// before its grace period (GraceTimer) runs out.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API ACityTaskActor : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	ACityTaskActor(); // Builds the mesh and components.

	// Which task this station represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	ECityTaskKind TaskKind = ECityTaskKind::Power;

	// Seconds the player has to attend the task once it turns critical.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float GracePeriodSeconds = 8.f;

	// Set by the game mode: stations only run on level 3.
	UPROPERTY(BlueprintReadOnly, Category = "CityTask")
	bool bIsActiveThisLevel = true;

	// Fired if the grace period runs out while the task is still critical.
	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnCityTaskFailed OnTaskFailed;

	// Spends one resource and resets the task, if the player can afford it.
	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void Attend();

	// Current condition (0-100), for the HUD.
	UFUNCTION(BlueprintPure, Category = "CityTask")
	float GetCondition() const;

	// Shows/hides the station and pauses its decay for the current level.
	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void SetActiveThisLevel(bool bActive);

	// Puts the station back to a full meter with no countdown, using the level's decay speed.
	void ResetForLevel(float DecayScale);

protected:
	virtual void BeginPlay() override; // Wires the meter to the grace timer.

	// Visible, solid station the player walks up to.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UStaticMeshComponent> TaskMesh;

	// The decaying condition meter.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCityTaskComponent> TaskComp;

	// Countdown started when the task turns critical.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCooldownComponent> GraceTimer;

private:
	// Runs when the meter first drops to the critical threshold.
	UFUNCTION()
	void HandleBecameCritical();

	// Runs when the player attends the station.
	UFUNCTION()
	void HandleAttended();

	// Runs when the grace period ends.
	UFUNCTION()
	void HandleGraceExpired();

	// Latched once failed so the failure only fires once.
	bool bFailed = false;
};
