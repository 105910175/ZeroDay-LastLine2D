#include "CooldownComponent.h"

// Ticks every frame so the countdown can advance. Nothing happens until StartCooldown.
UCooldownComponent::UCooldownComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Clamp to a tiny positive value so a zero duration can't divide by zero in GetProgress01.
void UCooldownComponent::StartCooldown(float InDuration, bool bInLoop)
{
	Duration = FMath::Max(InDuration, KINDA_SMALL_NUMBER);
	TimeRemaining = Duration;
	bLoop = bInLoop;
	bCooldownRunning = true;
}

// Cancelling is silent: OnExpired is not fired.
void UCooldownComponent::CancelCooldown()
{
	bCooldownRunning = false;
	TimeRemaining = 0.f;
}

// Fraction of the countdown that has elapsed (1 once finished or if no duration was set).
float UCooldownComponent::GetProgress01() const
{
	if (Duration <= 0.f)
	{
		return 1.f;
	}
	return 1.f - FMath::Clamp(TimeRemaining / Duration, 0.f, 1.f);
}

// Counts down each frame and broadcasts OnExpired when time runs out.
void UCooldownComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bCooldownRunning)
	{
		return;
	}

	TimeRemaining -= DeltaTime;
	if (TimeRemaining <= 0.f)
	{
		if (bLoop)
		{
			// Carry the overshoot into the next cycle so looping timers don't drift.
			TimeRemaining += Duration;
		}
		else
		{
			bCooldownRunning = false;
		}
		// Broadcast last so listeners can restart or cancel the timer safely.
		OnExpired.Broadcast();
	}
}
