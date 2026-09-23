#include "BreachableComponent.h"

UBreachableComponent::UBreachableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBreachableComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Status = EWallStatus::Safe;
}

void UBreachableComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f || Status == EWallStatus::Breached)
	{
		return;
	}

	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	RecalculateStatus();
}

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

	if (NewStatus != Status)
	{
		Status = NewStatus;
		OnStatusChanged.Broadcast(Status);
	}
}
