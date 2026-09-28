#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "CityTaskComponent.generated.h" // Unreal's generated reflection code (must be last include).

// Fired once when the condition first drops to the critical threshold.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskBecameCritical);
// Fired when the player attends the task and it resets.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskAttended);

// Validate & Prioritise pressure: a condition meter that decays over time.
// Reused by ACityTaskActor for the power/food/medic tasks.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UCityTaskComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UCityTaskComponent(); // Sets the component up to tick.

	// How much condition (0-100) is lost per second (before the level's decay scale).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float DecayRatePerSecond = 2.f;

	// Condition at or below which the task is considered critical.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float CriticalThreshold = 25.f;

	// Fired once when condition first drops to the critical threshold.
	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnTaskBecameCritical OnBecameCritical;

	// Fired when the player attends the task and it resets.
	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnTaskAttended OnAttended;

	// Resets condition to full and clears the critical flag.
	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void Attend();

	// Sets a new decay multiplier and puts the meter back to full, ready for a new level.
	void ResetTask(float NewDecayScale);

	// Current condition, 0-100.
	UFUNCTION(BlueprintPure, Category = "CityTask")
	float GetCondition() const { return Condition; }

	// True while the task is at or below the critical threshold.
	UFUNCTION(BlueprintPure, Category = "CityTask")
	bool IsCritical() const { return bIsCritical; }

protected:
	virtual void BeginPlay() override; // Starts the meter full.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // Decays the meter each frame.

private:
	// Current condition, 0-100.
	UPROPERTY(Transient)
	float Condition = 100.f;

	// True from the moment condition drops to the threshold until the task is attended.
	UPROPERTY(Transient)
	bool bIsCritical = false;

	// Level-wide multiplier on how fast the meter drains.
	UPROPERTY(Transient)
	float DecayScale = 1.f;
};
