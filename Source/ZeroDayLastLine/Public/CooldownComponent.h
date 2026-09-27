#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CooldownComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCooldownExpired);

// Generic countdown timer. Reused by AWallActor (remediation window), ACityTaskActor
// (critical grace period) and AThreatSpawnerActor (spawn interval) instead of each
// system rolling its own timer handling.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent))
class ZERODAYLASTLINE_API UCooldownComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCooldownComponent();

	// Fired when the countdown reaches zero (every cycle when looping).
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownExpired OnExpired;

	// Starts (or restarts) a countdown of InDuration seconds. If bInLoop is set it re-arms itself after each expiry.
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void StartCooldown(float InDuration, bool bInLoop = false);

	// Stops the countdown without firing OnExpired.
	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void CancelCooldown();

	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool IsRunning() const { return bCooldownRunning; }

	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetTimeRemaining() const { return TimeRemaining; }

	// 0 when just started, 1 when finished. Useful for progress bars.
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetProgress01() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Length of the current countdown, in seconds.
	UPROPERTY(Transient)
	float Duration = 0.f;

	// Seconds left on the current countdown.
	UPROPERTY(Transient)
	float TimeRemaining = 0.f;

	// True while a countdown is active. Not named bIsActive: UActorComponent already has that.
	UPROPERTY(Transient)
	bool bCooldownRunning = false;

	// Whether the countdown restarts automatically after expiring.
	UPROPERTY(Transient)
	bool bLoop = false;
};
