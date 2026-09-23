#include "WallActor.h"
#include "BreachableComponent.h"
#include "CooldownComponent.h"
#include "ThreatPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"

AWallActor::AWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMesh;

	// Blockout mesh so the wall is visible in-editor before real art exists.
	// A 100x100x100 engine cube, stretched into a wide low rampart shape.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		WallMesh->SetStaticMesh(CubeMeshAsset.Object);
	}
	WallMesh->SetRelativeScale3D(FVector(3.0f, 0.6f, 1.5f));

	WallMesh->SetCollisionObjectType(ECC_GameTraceChannel1); // Wall channel
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WallMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap); // Projectiles pass through wall geometry so the player can shoot at threats approaching from outside

	ThreatContactZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ThreatContactZone"));
	ThreatContactZone->SetupAttachment(RootComponent);
	ThreatContactZone->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	ThreatContactZone->SetCollisionObjectType(ECC_GameTraceChannel1); // Wall channel
	ThreatContactZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	ThreatContactZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap); // Threats

	Breachable = CreateDefaultSubobject<UBreachableComponent>(TEXT("Breachable"));
	RemediationTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("RemediationTimer"));
}

void AWallActor::BeginPlay()
{
	Super::BeginPlay();

	Breachable->OnStatusChanged.AddDynamic(this, &AWallActor::HandleStatusChanged);
	RemediationTimer->OnExpired.AddDynamic(this, &AWallActor::HandleRemediationExpired);
	ThreatContactZone->OnComponentBeginOverlap.AddDynamic(this, &AWallActor::HandleThreatContactBegin);
}

void AWallActor::TakeThreatDamage(float Amount)
{
	if (!bIsActiveThisLevel)
	{
		return;
	}
	Breachable->ApplyDamage(Amount);
}

void AWallActor::AttemptFix()
{
	if (Breachable->GetStatus() == EWallStatus::Breached)
	{
		Breachable->FullyRepair();
	}
}

EWallStatus AWallActor::GetStatus() const
{
	return Breachable->GetStatus();
}

void AWallActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive;
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
}

void AWallActor::HandleStatusChanged(EWallStatus NewStatus)
{
	if (NewStatus == EWallStatus::Breached)
	{
		RemediationTimer->StartCooldown(RemediationWindowSeconds, false);
	}
	else
	{
		RemediationTimer->CancelCooldown();
	}
}

void AWallActor::HandleRemediationExpired()
{
	if (Breachable->GetStatus() == EWallStatus::Breached)
	{
		OnWallLost.Broadcast(this);
	}
}

void AWallActor::HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AThreatPawn* Threat = Cast<AThreatPawn>(OtherActor))
	{
		Threat->OnReachedTargetWall();
	}
}
