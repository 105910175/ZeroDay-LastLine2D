#include "CityTaskComponent.h"

UCityTaskComponent::UCityTaskComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCityTaskComponent::BeginPlay()
{
	Super::BeginPlay();
	Condition = 100.f;
	bIsCritical = false;
}

// Decays the condition meter over time.
void UCityTaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Fully decayed already; nothing left to lose.
	if (Condition <= 0.f)
	{
		return;
	}

	Condition = FMath::Clamp(Condition - DecayRatePerSecond * DeltaTime, 0.f, 100.f);

	// Notify once, on the way down, so the owner can start its grace period.
	if (!bIsCritical && Condition <= CriticalThreshold)
	{
		bIsCritical = true;
		OnBecameCritical.Broadcast();
	}
}

// Attending fully restores the task.
void UCityTaskComponent::Attend()
{
	Condition = 100.f;
	bIsCritical = false;
	OnAttended.Broadcast();
}
