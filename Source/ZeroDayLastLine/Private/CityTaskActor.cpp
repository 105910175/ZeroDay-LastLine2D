#include "CityTaskActor.h"
#include "CityTaskComponent.h"
#include "CooldownComponent.h"
#include "ZeroDayGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

// Builds the station: a solid mesh plus the shared decay and timer components.
ACityTaskActor::ACityTaskActor()
{
	PrimaryActorTick.bCanEverTick = false;

	TaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaskMesh"));
	RootComponent = TaskMesh;

	// Blockout mesh: a chest-high crate/console stand-in for whichever task this is.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		TaskMesh->SetStaticMesh(CubeMeshAsset.Object);
	}
	TaskMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));

	TaskMesh->SetCollisionResponseToAllChannels(ECR_Block);

	TaskComp = CreateDefaultSubobject<UCityTaskComponent>(TEXT("TaskComp"));
	GraceTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("GraceTimer"));
}

// Wire the decay meter to the grace timer: critical starts it, attending cancels it, expiry fails the task.
void ACityTaskActor::BeginPlay()
{
	Super::BeginPlay();

	TaskComp->OnBecameCritical.AddDynamic(this, &ACityTaskActor::HandleBecameCritical);
	TaskComp->OnAttended.AddDynamic(this, &ACityTaskActor::HandleAttended);
	GraceTimer->OnExpired.AddDynamic(this, &ACityTaskActor::HandleGraceExpired);
}

// Attending costs a resource. If the player can't pay, nothing happens.
void ACityTaskActor::Attend()
{
	if (!bIsActiveThisLevel || bFailed)
	{
		return;
	}

	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (!GameMode->SpendResource(1))
		{
			return;
		}
	}

	TaskComp->Attend();
}

// Current condition (0-100), for the HUD.
float ACityTaskActor::GetCondition() const
{
	return TaskComp->GetCondition();
}

// Inactive tasks are hidden and stop decaying.
void ACityTaskActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive;
	SetActorHiddenInGame(!bActive);
	TaskComp->SetComponentTickEnabled(bActive);
}

// Condition hit the threshold: the player now has GracePeriodSeconds to respond.
void ACityTaskActor::HandleBecameCritical()
{
	GraceTimer->StartCooldown(GracePeriodSeconds, false);
}

// Player responded in time, so the grace period is cancelled.
void ACityTaskActor::HandleAttended()
{
	GraceTimer->CancelCooldown();
}

// Ran out of time while still critical: the task has failed.
void ACityTaskActor::HandleGraceExpired()
{
	if (TaskComp->IsCritical() && !bFailed)
	{
		bFailed = true;
		OnTaskFailed.Broadcast(this);
	}
}
