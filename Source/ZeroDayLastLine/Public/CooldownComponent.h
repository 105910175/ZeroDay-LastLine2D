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

	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FOnCooldownExpired OnExpired;

	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void StartCooldown(float InDuration, bool bInLoop = false);

	UFUNCTION(BlueprintCallable, Category = "Cooldown")
	void CancelCooldown();

	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool IsRunning() const { return bCooldownRunning; }

	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetTimeRemaining() const { return TimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetProgress01() const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	float Duration = 0.f;

	UPROPERTY(Transient)
	float TimeRemaining = 0.f;

	UPROPERTY(Transient)
	bool bCooldownRunning = false;

	UPROPERTY(Transient)
	bool bLoop = false;
};
