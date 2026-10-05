#include "BreachableComponent.h" // This class's own declaration.

// No tick needed: health only changes when damage or repair is applied.
UBreachableComponent::UBreachableComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Nothing to do every frame.
}

// Start at full health, fully secure.
void UBreachableComponent::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.
	Health = MaxHealth; // Begin at full health.
	Status = EWallStatus::Safe; // And fully secure.
}

// Takes damage and updates the status.
void UBreachableComponent::ApplyDamage(float Amount)
{
	// A breached wall takes no further damage until it has been repaired.
	if (Amount <= 0.f || Status == EWallStatus::Breached) // Nothing to apply, or already broken.
	{
		return; // Ignore it.
	}

	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth); // Lose health, never below zero.
	RecalculateStatus(); // Work out whether the status changed.
}

// Repairing can move a breached wall back to Threatened or Safe depending on how much is restored.
void UBreachableComponent::Repair(float Amount)
{
	if (Amount <= 0.f) // Nothing to restore.
	{
		return; // Ignore it.
	}

	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth); // Gain health, never above the maximum.
	RecalculateStatus(); // Work out whether the status changed.
}

// Restores everything: used when the player patches a breach.
void UBreachableComponent::FullyRepair()
{
	Health = MaxHealth; // Back to full health.
	RecalculateStatus(); // Return to Safe (and announce it).
}

// Health -> status: zero is Breached, at or below the threshold is Threatened, otherwise Safe.
void UBreachableComponent::RecalculateStatus()
{
	EWallStatus NewStatus = Status; // Start from the current status.

	if (Health <= 0.f) // No health left?
	{
		NewStatus = EWallStatus::Breached; // The wall is broken.
	}
	else if (Health / MaxHealth <= ThreatenedHealthPercent) // At or below the warning threshold?
	{
		NewStatus = EWallStatus::Threatened; // Under attack.
	}
	else
	{
		NewStatus = EWallStatus::Safe; // Plenty of health left.
	}

	// Only broadcast on a real change so listeners aren't spammed every hit.
	if (NewStatus != Status) // Did the status actually change?
	{
		Status = NewStatus; // Store the new status.
		OnStatusChanged.Broadcast(Status); // Tell the wall (it updates artwork, alerts and the countdown).
	}
}
