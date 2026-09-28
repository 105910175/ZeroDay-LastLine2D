#include "CityTaskComponent.h" // This class's own declaration.

// Ticks every frame so the meter can drain smoothly.
UCityTaskComponent::UCityTaskComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // Allow TickComponent to run.
}

// Start full and not critical when the game begins.
void UCityTaskComponent::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.
	Condition = 100.f; // Begin at full condition.
	bIsCritical = false; // Not critical yet.
}

// Decays the condition meter over time.
void UCityTaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction); // Run the base class tick first.

	// Fully decayed already; nothing left to lose.
	if (Condition <= 0.f) // Already empty?
	{
		return; // Nothing more to drain.
	}

	Condition = FMath::Clamp(Condition - DecayRatePerSecond * DecayScale * DeltaTime, 0.f, 100.f); // Drain scaled by the level, kept within 0-100.

	// Notify once, on the way down, so the owner can start its grace period.
	if (!bIsCritical && Condition <= CriticalThreshold) // First time we dropped to the threshold?
	{
		bIsCritical = true; // Latch critical so we only announce once.
		OnBecameCritical.Broadcast(); // Tell the station to start its grace timer.
	}
}

// Attending fully restores the task.
void UCityTaskComponent::Attend()
{
	Condition = 100.f; // Back to full.
	bIsCritical = false; // No longer critical.
	OnAttended.Broadcast(); // Tell the station to cancel its grace timer.
}

// Used when a level starts: full meter, and the new level's decay speed.
void UCityTaskComponent::ResetTask(float NewDecayScale)
{
	DecayScale = NewDecayScale; // Remember the level's decay multiplier.
	Condition = 100.f; // Back to full.
	bIsCritical = false; // Not critical.
}
