#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "CooldownComponent.generated.h" // Unreal's generated reflection code (must be last include).

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCooldownExpired); // Fired when the countdown reaches zero.

// Generic countdown timer. Reused by AWallActor (remediation window), ACityTaskActor
// (critical grace period) and AThreatSpawnerActor (spawn interval) instead of each
// system rolling its own timer handling.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UCooldownComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UCooldownComponent(); // Turns ticking on; nothing counts until StartCooldown.

	// Fired when the countdown reaches zero (every cycle when looping).
	UPROPERTY(BlueprintAssignable, Category = "Cooldown") // Other classes can subscribe to it.
	FOnCooldownExpired OnExpired; // Listeners react when time is up.

	// Starts (or restarts) a countdown of InDuration seconds. If bInLoop is set it re-arms itself after each expiry.
	UFUNCTION(BlueprintCallable, Category = "Cooldown") // Callable from Blueprints.
	void StartCooldown(float InDuration, bool bInLoop = false); // Begin counting down.

	// Stops the countdown without firing OnExpired.
	UFUNCTION(BlueprintCallable, Category = "Cooldown") // Callable from Blueprints.
	void CancelCooldown(); // Silent stop.

	UFUNCTION(BlueprintPure, Category = "Cooldown") // A read-only getter.
	bool IsRunning() const { return bCooldownRunning; } // True while counting down.

	UFUNCTION(BlueprintPure, Category = "Cooldown") // A read-only getter.
	float GetTimeRemaining() const { return TimeRemaining; } // Seconds left.

	// 0 when just started, 1 when finished. Useful for progress bars.
	UFUNCTION(BlueprintPure, Category = "Cooldown") // A read-only getter.
	float GetProgress01() const; // Fraction elapsed.

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // Counts down each frame.

private:
	// Length of the current countdown, in seconds.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	float Duration = 0.f; // Total length.

	// Seconds left on the current countdown.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	float TimeRemaining = 0.f; // Time left.

	// True while a countdown is active. Not named bIsActive: UActorComponent already has that.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	bool bCooldownRunning = false; // Whether it is counting.

	// Whether the countdown restarts automatically after expiring.
	UPROPERTY(Transient) // Not saved; set fresh each run.
	bool bLoop = false; // True for looping timers.
};
