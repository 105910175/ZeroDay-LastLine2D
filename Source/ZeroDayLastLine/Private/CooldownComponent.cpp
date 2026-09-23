#include "CooldownComponent.h"

UCooldownComponent::UCooldownComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCooldownComponent::StartCooldown(float InDuration, bool bInLoop)
{
	Duration = FMath::Max(InDuration, KINDA_SMALL_NUMBER);
	TimeRemaining = Duration;
	bLoop = bInLoop;
	bCooldownRunning = true;
}

void UCooldownComponent::CancelCooldown()
{
	bCooldownRunning = false;
	TimeRemaining = 0.f;
}

float UCooldownComponent::GetProgress01() const
{
	if (Duration <= 0.f)
	{
		return 1.f;
	}
	return 1.f - FMath::Clamp(TimeRemaining / Duration, 0.f, 1.f);
}

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
			TimeRemaining += Duration;
		}
		else
		{
			bCooldownRunning = false;
		}
		OnExpired.Broadcast();
	}
}
