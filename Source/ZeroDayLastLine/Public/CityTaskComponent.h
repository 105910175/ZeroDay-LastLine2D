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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float DecayRatePerSecond = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float CriticalThreshold = 25.f;

	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnTaskBecameCritical OnBecameCritical;

	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnTaskAttended OnAttended;

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
	UPROPERTY(Transient)
	float Condition = 100.f;

	UPROPERTY(Transient)
	bool bIsCritical = false;
};
