#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "ZeroDayTypes.h" // EWallStatus.
#include "BreachableComponent.generated.h" // Unreal's generated reflection code (must be last include).

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallStatusChanged, EWallStatus, NewStatus); // Fired when the status changes; carries the new one.

// Detect stage in miniature: tracks health/status for anything that can be
// damaged down to a breach (currently AWallActor). Kept separate from AWallActor
// so the same health/status logic could be reused on another actor if needed.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UBreachableComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UBreachableComponent(); // Turns ticking off; health only changes on damage or repair.

	// Starting and maximum health.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable") // Editable in the editor and Blueprints.
	float MaxHealth = 100.f; // A full wall.

	// Health fraction (0-1) at or below which the status becomes Threatened.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachable") // Editable in the editor and Blueprints.
	float ThreatenedHealthPercent = 0.5f; // Half health and below is Threatened.

	// Fired only when the status actually changes (Safe / Threatened / Breached).
	UPROPERTY(BlueprintAssignable, Category = "Breachable") // Other classes can subscribe to it.
	FOnWallStatusChanged OnStatusChanged; // The wall listens to this.

	// Reduces health. Ignored once breached; the wall must be repaired first.
	UFUNCTION(BlueprintCallable, Category = "Breachable") // Callable from Blueprints.
	void ApplyDamage(float Amount); // Called when a threat reaches the wall.

	// Restores some health and re-evaluates the status.
	UFUNCTION(BlueprintCallable, Category = "Breachable") // Callable from Blueprints.
	void Repair(float Amount); // Partial repair.

	// Restores full health. Used when the player fixes a breach.
	UFUNCTION(BlueprintCallable, Category = "Breachable") // Callable from Blueprints.
	void FullyRepair(); // Complete repair.

	UFUNCTION(BlueprintPure, Category = "Breachable") // A read-only getter.
	EWallStatus GetStatus() const { return Status; } // Safe, Threatened or Breached.

	UFUNCTION(BlueprintPure, Category = "Breachable") // A read-only getter.
	float GetHealthPercent() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; } // 0-1 health, safe against a zero maximum.

protected:
	virtual void BeginPlay() override; // Starts at full health.

private:
	// Derives Safe/Threatened/Breached from current health and broadcasts on change.
	void RecalculateStatus(); // Called after every health change.

	UPROPERTY(Transient) // Not saved; set fresh each run.
	float Health = 0.f; // Current health.

	UPROPERTY(Transient) // Not saved; set fresh each run.
	EWallStatus Status = EWallStatus::Safe; // Current status.
};
