#include "CooldownComponent.h" // This class's own declaration.

// Ticks every frame so the countdown can advance. Nothing happens until StartCooldown.
UCooldownComponent::UCooldownComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // Allow TickComponent to run.
}

// Clamp to a tiny positive value so a zero duration can't divide by zero in GetProgress01.
void UCooldownComponent::StartCooldown(float InDuration, bool bInLoop)
{
	Duration = FMath::Max(InDuration, KINDA_SMALL_NUMBER); // Never zero or negative.
	TimeRemaining = Duration; // Start the clock full.
	bLoop = bInLoop; // Remember whether to restart after expiring.
	bCooldownRunning = true; // Begin counting down.
}

// Cancelling is silent: OnExpired is not fired.
void UCooldownComponent::CancelCooldown()
{
	bCooldownRunning = false; // Stop counting.
	TimeRemaining = 0.f; // Nothing left on the clock.
}

// Fraction of the countdown that has elapsed (1 once finished or if no duration was set).
float UCooldownComponent::GetProgress01() const
{
	if (Duration <= 0.f) // No countdown was ever set.
	{
		return 1.f; // Treat it as finished.
	}
	return 1.f - FMath::Clamp(TimeRemaining / Duration, 0.f, 1.f); // 0 when just started, 1 when done.
}

// Counts down each frame and broadcasts OnExpired when time runs out.
void UCooldownComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction); // Run the base class tick first.

	if (!bCooldownRunning) // Nothing is counting down.
	{
		return; // Do nothing.
	}

	TimeRemaining -= DeltaTime; // Spend this frame's time.
	if (TimeRemaining <= 0.f) // Ran out?
	{
		if (bLoop) // Looping timers restart themselves.
		{
			// Carry the overshoot into the next cycle so looping timers don't drift.
			TimeRemaining += Duration; // Start the next cycle.
		}
		else
		{
			bCooldownRunning = false; // One-shot timers stop here.
		}
		// Broadcast last so listeners can restart or cancel the timer safely.
		OnExpired.Broadcast(); // Tell everyone listening that time is up.
	}
}
