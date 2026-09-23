#include "ThreatPawn.h"
#include "WallActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AThreatPawn::AThreatPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	RootComponent = HitSphere;
	HitSphere->SetSphereRadius(50.f);
	HitSphere->SetCollisionObjectType(ECC_GameTraceChannel2); // Threat channel
	HitSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	HitSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // Physically stop at wall meshes

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Blockout mesh: an upright cylinder reads as a humanoid silhouette better than a sphere.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshAsset.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMeshAsset.Object);
	}
	BodyMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.0f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
}

void AThreatPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AThreatPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetWall)
	{
		return;
	}

	const FVector ToTarget = TargetWall->GetActorLocation() - GetActorLocation();
	const FVector FlatDirection = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();

	if (!FlatDirection.IsNearlyZero())
	{
		AddActorWorldOffset(FlatDirection * MoveSpeed * DeltaTime, true);
		SetActorRotation(FlatDirection.Rotation());
	}
}

void AThreatPawn::ApplyHitDamage(float Amount)
{
	Health -= Amount;
	if (Health <= 0.f)
	{
		Destroy();
	}
}

void AThreatPawn::OnReachedTargetWall()
{
	if (TargetWall)
	{
		TargetWall->TakeThreatDamage(ContactDamage);
	}
	Destroy();
}
