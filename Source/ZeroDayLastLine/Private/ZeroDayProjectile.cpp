#include "ZeroDayProjectile.h" // This class's own declaration.
#include "ThreatPawn.h" // The thing shots are meant to hit.
#include "ZeroDayPhysics.h" // The projectile's physics material.
#include "ZeroDaySprites.h" // Helper that lays sprites flat.
#include "PaperSprite.h" // The bolt artwork asset type.
#include "PaperSpriteComponent.h" // Draws the bolt.
#include "Components/SphereComponent.h" // The collision body.
#include "Components/StaticMeshComponent.h" // The blockout visual.
#include "GameFramework/ProjectileMovementComponent.h" // Moves the shot.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine and content assets.

// Removes itself after InitialLifeSpan seconds so missed shots don't pile up.
AZeroDayProjectile::AZeroDayProjectile()
{
	PrimaryActorTick.bCanEverTick = false; // The movement component moves it; no per-frame logic needed.
	InitialLifeSpan = 3.f; // Delete the shot after three seconds if it hits nothing.

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere")); // The collision body.
	RootComponent = CollisionSphere; // It is the root everything else follows.
	CollisionSphere->SetSphereRadius(15.f); // A small round shot.
	CollisionSphere->SetCollisionObjectType(ECC_GameTraceChannel3); // Projectile channel (GameTraceChannel3).
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore); // Ignore everything by default...
	CollisionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap); // ...overlap threats so a hit can damage them...
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // ...stop at generic level geometry, but pass through our Wall channel so players can shoot at threats approaching from outside...
	CollisionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Block); // ...and knock resource pickups around (pickups block projectiles too).
	CollisionSphere->SetNotifyRigidBodyCollision(true); // Needed for the hit event to fire on blocking contacts.

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh")); // The blockout visual.
	ProjectileMesh->SetupAttachment(RootComponent); // Attach it to the body.
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // It is only for looks.

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere")); // Find the engine sphere once.
	if (SphereMeshAsset.Succeeded()) // Only use it if it was found.
	{
		ProjectileMesh->SetStaticMesh(SphereMeshAsset.Object); // Show the sphere.
	}
	ProjectileMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.15f)); // Small: 15 units across.

	BoltSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BoltSprite")); // The plasma bolt artwork.
	BoltSprite->SetupAttachment(RootComponent); // It travels with the shot.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> BoltFinder(TEXT("/Game/Sprites/S_bolt.S_bolt")); // Find the artwork once.
	if (BoltFinder.Succeeded()) // If it exists...
	{
		ZeroDaySprites::ConfigureFlat(BoltSprite, BoltFinder.Object, ZeroDaySprites::HeadingRotation(), 12.f); // ...lay it flat pointing along the flight path...
		ProjectileMesh->SetVisibility(false); // ...and hide the blockout sphere.
	}

	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp")); // Flies the shot.
	MovementComp->InitialSpeed = Speed; // Start at full speed.
	MovementComp->MaxSpeed = Speed; // Never speed up.
	MovementComp->bRotationFollowsVelocity = true; // Turn to face the direction of travel (the bolt art follows).
	MovementComp->ProjectileGravityScale = 0.f; // Top-down game: no gravity.
}

// Bind here so the components are ready.
void AZeroDayProjectile::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	CollisionSphere->SetPhysMaterialOverride(ZeroDayPhysics::Get(EZeroDayMaterial::Projectile)); // Frictionless, bouncy plasma.
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AZeroDayProjectile::HandleOverlap); // Threat contact.
	CollisionSphere->OnComponentHit.AddDynamic(this, &AZeroDayProjectile::HandleHit); // Solid contact.
}

// Hitting a threat damages it and consumes the shot.
void AZeroDayProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AThreatPawn* Threat = Cast<AThreatPawn>(OtherActor)) // Did we touch a threat?
	{
		Threat->ApplyHitDamage(Damage); // Hurt it.
		Destroy(); // The shot is used up.
	}
}

// Only fires for things the projectile blocks on (static level geometry and pickups): stop there.
void AZeroDayProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherComp && OtherComp->IsSimulatingPhysics()) // Did we hit something that moves with physics (a pickup)?
	{
		const FVector Direction = MovementComp ? MovementComp->Velocity.GetSafeNormal2D() : FVector::ZeroVector; // The way we were travelling.
		OtherComp->AddImpulse(Direction * PickupKnockSpeed, NAME_None, true); // Knock it along (a velocity change, so light and heavy pickups both move).
	}
	Destroy(); // The shot is used up.
}
