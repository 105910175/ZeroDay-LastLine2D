#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "CityTaskComponent.generated.h" // Unreal's generated reflection code (must be last include).

// Fired once when the condition first drops to the critical threshold.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskBecameCritical); // No parameters.
// Fired when the player attends the task and it resets.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskAttended); // No parameters.

// Validate & Prioritise pressure: a condition meter that decays over time.
// Reused by ACityTaskActor for the power/food/medic tasks.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UCityTaskComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UCityTaskComponent(); // Sets the component up to tick.

	// How much condition (0-100) is lost per second (before the level's decay scale).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask") // Editable in the editor and Blueprints.
	float DecayRatePerSecond = 2.f; // Base drain speed.

	// Condition at or below which the task is considered critical.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask") // Editable in the editor and Blueprints.
	float CriticalThreshold = 25.f; // A quarter of the meter.

	// Fired once when condition first drops to the critical threshold.
	UPROPERTY(BlueprintAssignable, Category = "CityTask") // Other classes can subscribe to it.
	FOnTaskBecameCritical OnBecameCritical; // The station starts its grace timer.

	// Fired when the player attends the task and it resets.
	UPROPERTY(BlueprintAssignable, Category = "CityTask") // Other classes can subscribe to it.
	FOnTaskAttended OnAttended; // The station cancels its grace timer.

	// Resets condition to full and clears the critical flag.
	UFUNCTION(BlueprintCallable, Category = "CityTask") // Callable from Blueprints.
	void Attend(); // Full restore.

	// Sets a new decay multiplier and puts the meter back to full, ready for a new level.
	void ResetTask(float NewDecayScale); // Called at the start of every level.

	// Puts the meter back to a given value (0-100) and clears the critical flag, keeping the decay speed.
	void RestoreCondition(float NewCondition); // Partial restore after a failure.

	// Current condition, 0-100.
	UFUNCTION(BlueprintPure, Category = "CityTask") // A read-only getter.
	float GetCondition() const { return Condition; } // The meter's value.

	// True while the task is at or below the critical threshold.
	UFUNCTION(BlueprintPure, Category = "CityTask") // A read-only getter.
	bool IsCritical() const { return bIsCritical; } // The critical flag.

protected:
	virtual void BeginPlay() override; // Starts the meter full.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // Decays the meter each frame.

private:
	// Current condition, 0-100.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	float Condition = 100.f; // Starts full.

	// True from the moment condition drops to the threshold until the task is attended.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	bool bIsCritical = false; // Latched so the critical event only fires once.

	// Level-wide multiplier on how fast the meter drains.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	float DecayScale = 1.f; // Set by ResetTask.
};
