#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZeroDayTypes.h"
#include "BreachableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallStatusChanged, EWallStatus, NewStatus);

// Detect stage in miniature: tracks health/status for anything that can be
// damaged down to a breach (currently AWallActor). Kept separate from AWallActor
// so the same health/status logic could be reused on another actor if needed.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent))
class ZERODAYLASTLINE_API UBreachableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBreachableComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable")
	float ThreatenedHealthPercent = 0.5f;

	UPROPERTY(BlueprintAssignable, Category = "Breachable")
	FOnWallStatusChanged OnStatusChanged;

	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void Repair(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void FullyRepair();

	UFUNCTION(BlueprintPure, Category = "Breachable")
	EWallStatus GetStatus() const { return Status; }

	UFUNCTION(BlueprintPure, Category = "Breachable")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	void RecalculateStatus();

	UPROPERTY(Transient)
	float Health = 0.f;

	UPROPERTY(Transient)
	EWallStatus Status = EWallStatus::Safe;
};
