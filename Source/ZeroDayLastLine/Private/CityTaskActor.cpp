#include "CityTaskActor.h" // This class's own declaration.
#include "CityTaskComponent.h" // The decaying meter.
#include "CooldownComponent.h" // The grace timer.
#include "ZeroDayGameMode.h" // Where resources are spent.
#include "Components/StaticMeshComponent.h" // The visible station.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine assets.

// Builds the station: a solid mesh plus the shared decay and timer components.
ACityTaskActor::ACityTaskActor()
{
	PrimaryActorTick.bCanEverTick = false; // The components do the timing, not the actor.

	TaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaskMesh")); // The visible station body.
	RootComponent = TaskMesh; // Make it the root of the actor.

	// Blockout mesh: a chest-high crate/console stand-in for whichever task this is.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube")); // Find the engine cube once.
	if (CubeMeshAsset.Succeeded()) // Only use it if it was found.
	{
		TaskMesh->SetStaticMesh(CubeMeshAsset.Object); // Show the cube.
	}
	TaskMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f)); // Slightly narrower than a full cube.

	TaskMesh->SetCollisionResponseToAllChannels(ECR_Block); // Solid: the player walks around it.

	TaskComp = CreateDefaultSubobject<UCityTaskComponent>(TEXT("TaskComp")); // The decaying condition meter.
	GraceTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("GraceTimer")); // The shared countdown timer.
}

// Wire the decay meter to the grace timer: critical starts it, attending cancels it, expiry fails the task.
void ACityTaskActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	TaskComp->OnBecameCritical.AddDynamic(this, &ACityTaskActor::HandleBecameCritical); // Meter critical -> start the countdown.
	TaskComp->OnAttended.AddDynamic(this, &ACityTaskActor::HandleAttended); // Attended -> stop the countdown.
	GraceTimer->OnExpired.AddDynamic(this, &ACityTaskActor::HandleGraceExpired); // Countdown over -> the task fails.
}

// Attending costs a resource. If the player can't pay, nothing happens.
void ACityTaskActor::Attend()
{
	if (!bIsActiveThisLevel || bFailed) // Not in play, or already failed.
	{
		return; // Ignore the request.
	}

	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
	{
		if (!GameMode->SpendResource(1)) // Try to pay one resource.
		{
			return; // Not enough resources.
		}
	}

	TaskComp->Attend(); // Paid up, so restore the station.
}

// Current condition (0-100), for the HUD.
float ACityTaskActor::GetCondition() const
{
	return TaskComp->GetCondition(); // Read the meter.
}

// Inactive stations are hidden and stop decaying.
void ACityTaskActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive; // Remember the state for other systems.
	SetActorHiddenInGame(!bActive); // Hide the station when it isn't in play.
	TaskComp->SetComponentTickEnabled(bActive); // Only decay while in play.
}

// Puts the station back to a clean state for a fresh level.
void ACityTaskActor::ResetForLevel(float DecayScale)
{
	bFailed = false; // Allow it to run (and fail) again.
	GraceTimer->CancelCooldown(); // No leftover countdown.
	TaskComp->ResetTask(DecayScale); // Full meter at the new level's decay speed.
}

// Condition hit the threshold: the player now has GracePeriodSeconds to respond.
void ACityTaskActor::HandleBecameCritical()
{
	GraceTimer->StartCooldown(GracePeriodSeconds, false); // Start the one-shot countdown.
}

// Player responded in time, so the grace period is cancelled.
void ACityTaskActor::HandleAttended()
{
	GraceTimer->CancelCooldown(); // Stop the countdown.
}

// Ran out of time while still critical: the task has failed.
void ACityTaskActor::HandleGraceExpired()
{
	if (TaskComp->IsCritical() && !bFailed) // Still critical and not already failed?
	{
		bFailed = true; // Latch so we only fail once.
		OnTaskFailed.Broadcast(this); // Tell the game mode.
	}
}
