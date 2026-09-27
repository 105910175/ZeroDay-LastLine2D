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

	// Starting and maximum health.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable")
	float MaxHealth = 100.f;

	// Health fraction (0-1) at or below which the status becomes Threatened.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable")
	float ThreatenedHealthPercent = 0.5f;

	// Fired only when the status actually changes (Safe / Threatened / Breached).
	UPROPERTY(BlueprintAssignable, Category = "Breachable")
	FOnWallStatusChanged OnStatusChanged;

	// Reduces health. Ignored once breached; the wall must be repaired first.
	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void ApplyDamage(float Amount);

	// Restores some health and re-evaluates the status.
	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void Repair(float Amount);

	// Restores full health. Used when the player fixes a breach.
	UFUNCTION(BlueprintCallable, Category = "Breachable")
	void FullyRepair();

	UFUNCTION(BlueprintPure, Category = "Breachable")
	EWallStatus GetStatus() const { return Status; }

	UFUNCTION(BlueprintPure, Category = "Breachable")
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	// Derives Safe/Threatened/Breached from current health and broadcasts on change.
	void RecalculateStatus();

	UPROPERTY(Transient)
	float Health = 0.f;

	UPROPERTY(Transient)
	EWallStatus Status = EWallStatus::Safe;
};
