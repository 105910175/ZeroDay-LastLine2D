#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CityTaskComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskBecameCritical);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskAttended);

// Validate & Prioritise pressure: a condition meter that decays over time.
// Reused by ACityTaskActor for the power/food/medic tasks.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent))
class ZERODAYLASTLINE_API UCityTaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCityTaskComponent();

	// How much condition (0-100) is lost per second.
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

	UFUNCTION(BlueprintPure, Category = "CityTask")
	float GetCondition() const { return Condition; }

	UFUNCTION(BlueprintPure, Category = "CityTask")
	bool IsCritical() const { return bIsCritical; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Current condition, 0-100.
	UPROPERTY(Transient)
	float Condition = 100.f;

	// True from the moment condition drops to the threshold until the task is attended.
	UPROPERTY(Transient)
	bool bIsCritical = false;
};
