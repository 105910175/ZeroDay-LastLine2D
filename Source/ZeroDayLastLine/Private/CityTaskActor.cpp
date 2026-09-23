#include "CityTaskActor.h"
#include "CityTaskComponent.h"
#include "CooldownComponent.h"
#include "ZeroDayGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

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

void ACityTaskActor::BeginPlay()
{
	Super::BeginPlay();

	TaskComp->OnBecameCritical.AddDynamic(this, &ACityTaskActor::HandleBecameCritical);
	TaskComp->OnAttended.AddDynamic(this, &ACityTaskActor::HandleAttended);
	GraceTimer->OnExpired.AddDynamic(this, &ACityTaskActor::HandleGraceExpired);
}

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

float ACityTaskActor::GetCondition() const
{
	return TaskComp->GetCondition();
}

void ACityTaskActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive;
	SetActorHiddenInGame(!bActive);
	TaskComp->SetComponentTickEnabled(bActive);
}

void ACityTaskActor::HandleBecameCritical()
{
	GraceTimer->StartCooldown(GracePeriodSeconds, false);
}

void ACityTaskActor::HandleAttended()
{
	GraceTimer->CancelCooldown();
}

void ACityTaskActor::HandleGraceExpired()
{
	if (TaskComp->IsCritical() && !bFailed)
	{
		bFailed = true;
		OnTaskFailed.Broadcast(this);
	}
}
