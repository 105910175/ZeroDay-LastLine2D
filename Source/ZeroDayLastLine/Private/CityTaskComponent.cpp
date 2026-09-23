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

void UCityTaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Condition <= 0.f)
	{
		return;
	}

	Condition = FMath::Clamp(Condition - DecayRatePerSecond * DeltaTime, 0.f, 100.f);

	if (!bIsCritical && Condition <= CriticalThreshold)
	{
		bIsCritical = true;
		OnBecameCritical.Broadcast();
	}
}

void UCityTaskComponent::Attend()
{
	Condition = 100.f;
	bIsCritical = false;
	OnAttended.Broadcast();
}
