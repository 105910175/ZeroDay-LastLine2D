#include "BreachableComponent.h"

// No tick needed: health only changes when damage or repair is applied.
UBreachableComponent::UBreachableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Start at full health, fully secure.
void UBreachableComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Status = EWallStatus::Safe;
}

void UBreachableComponent::ApplyDamage(float Amount)
{
	// A breached wall takes no further damage until it has been repaired.
	if (Amount <= 0.f || Status == EWallStatus::Breached)
	{
		return;
	}

	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	RecalculateStatus();
}

// Repairing can move a breached wall back to Threatened or Safe depending on how much is restored.
void UBreachableComponent::Repair(float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}

	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	RecalculateStatus();
}

void UBreachableComponent::FullyRepair()
{
	Health = MaxHealth;
	RecalculateStatus();
}

// Health -> status: zero is Breached, at or below the threshold is Threatened, otherwise Safe.
void UBreachableComponent::RecalculateStatus()
{
	EWallStatus NewStatus = Status;

	if (Health <= 0.f)
	{
		NewStatus = EWallStatus::Breached;
	}
	else if (Health / MaxHealth <= ThreatenedHealthPercent)
	{
		NewStatus = EWallStatus::Threatened;
	}
	else
	{
		NewStatus = EWallStatus::Safe;
	}

	// Only broadcast on a real change so listeners aren't spammed every hit.
	if (NewStatus != Status)
	{
		Status = NewStatus;
		OnStatusChanged.Broadcast(Status);
	}
}
