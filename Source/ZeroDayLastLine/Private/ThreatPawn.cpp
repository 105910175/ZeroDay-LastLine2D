#include "ThreatPawn.h" // This class's own declaration.
#include "WallActor.h" // So the threat can hurt and unregister from its wall.
#include "ZeroDayGameMode.h" // So the game mode can record when a threat is shot.
#include "ZeroDayPhysics.h" // The threat's physics material.
#include "ZeroDaySprites.h" // Helper that lays sprites flat.
#include "PaperSprite.h" // The artwork asset type.
#include "PaperSpriteComponent.h" // Draws the artwork.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "Components/SphereComponent.h" // The collision body.
#include "Components/StaticMeshComponent.h" // The visible stand-in.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine assets.

// Threat channel (GameTraceChannel2). Overlaps most things so shots can hit it, but blocks on wall meshes.
AThreatPawn::AThreatPawn()
{
	PrimaryActorTick.bCanEverTick = true; // Threats move every frame.

	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere")); // Create the collision body.
	RootComponent = HitSphere; // The sphere is the root that everything attaches to.
	HitSphere->SetSphereRadius(50.f); // Fifty units is roughly body sized.
	HitSphere->SetCollisionObjectType(ECC_GameTraceChannel2); // Report as the Threat channel.
	HitSphere->SetCollisionResponseToAllChannels(ECR_Overlap); // Overlap most things so shots can hit it.
	HitSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // But stop physically at wall meshes.

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh")); // Create the visible body.
	BodyMesh->SetupAttachment(RootComponent); // Attach it to the collision sphere.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // The mesh is only for looks.

	// Blockout mesh: an upright cylinder reads as a humanoid silhouette better than a sphere.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); // Find the engine cylinder once.
	if (CylinderMeshAsset.Succeeded()) // Only use it if it was found.
	{
		BodyMesh->SetStaticMesh(CylinderMeshAsset.Object); // Show the cylinder.
	}
	BodyMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.0f)); // Thin and tall.
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -50.f)); // Sit it so its base is at ground level.

	BodySprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodySprite")); // The zombie artwork.
	BodySprite->SetupAttachment(RootComponent); // It moves and turns with the threat.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> SpriteFinder(TEXT("/Game/Sprites/S_threat.S_threat")); // Find the artwork once.
	if (SpriteFinder.Succeeded()) // If it exists...
	{
		ZeroDaySprites::ConfigureFlat(BodySprite, SpriteFinder.Object, ZeroDaySprites::HeadingRotation(), 4.f); // ...lay it flat, facing the walking direction...
		BodyMesh->SetVisibility(false); // ...and hide the blockout cylinder.
	}
}

// Nothing extra to do when the threat appears.
void AThreatPawn::BeginPlay()
{
	Super::BeginPlay(); // Let the base class finish starting up.
	HitSphere->SetPhysMaterialOverride(ZeroDayPhysics::Get(EZeroDayMaterial::Threat)); // Slippery, non-bouncy body.
}

// Tell the wall this threat is gone so scans and validation stay accurate.
void AThreatPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TargetWall) // Only if we were heading for a wall.
	{
		TargetWall->UnregisterInboundThreat(this); // Remove us from that wall's list.
	}
	Super::EndPlay(EndPlayReason); // Let the base class finish cleaning up.
}

// Applies the target, severity and decoy status, and scales stats to match.
void AThreatPawn::InitialiseThreat(AWallActor* InTargetWall, EThreatSeverity InSeverity, bool bInIsDecoy, float SpeedScale, float HealthScale)
{
	TargetWall = InTargetWall; // Remember which wall to attack.
	Severity = InSeverity; // Remember how dangerous this threat is.
	bIsDecoy = bInIsDecoy; // Remember whether it is a harmless false positive.

	// Severity changes how hard it hits and how tough it is, relative to the base stats on the class.
	float DamageScale = 1.f; // Multiplier for contact damage.
	float SeverityHealthScale = 1.f; // Multiplier for health from severity.
	switch (Severity) // Choose multipliers by severity.
	{
	case EThreatSeverity::Low: // Weak threats.
		DamageScale = 0.5f; // Half damage.
		SeverityHealthScale = 0.7f; // Less health.
		break; // Done with Low.
	case EThreatSeverity::Critical: // Dangerous threats.
		DamageScale = 2.f; // Double damage.
		SeverityHealthScale = 2.f; // Double health.
		break; // Done with Critical.
	default: // Medium keeps the base stats.
		break; // Nothing to change.
	}

	ContactDamage = bIsDecoy ? 0.f : ContactDamage * DamageScale; // False positives are harmless.
	Health *= SeverityHealthScale * HealthScale; // Apply severity and level health scaling.
	MoveSpeed *= SpeedScale; // Apply the level's speed scaling.
	MaxHealth = Health; // Remember the starting health for the HUD's health bar.

	if (TargetWall) // If we have a wall to attack...
	{
		TargetWall->RegisterInboundThreat(this); // ...let it know we are coming so scans can find us.
	}
}

// Walks in a straight line at the target wall.
void AThreatPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // Run the base class tick first.

	if (!TargetWall) // No target means nowhere to go.
	{
		return; // Stand still.
	}

	const FVector ToTarget = TargetWall->GetActorLocation() - GetActorLocation(); // Vector from us to the wall.
	const FVector FlatDirection = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal(); // Ignore height so threats stay on the ground plane.

	if (!FlatDirection.IsNearlyZero()) // Only move if we have a direction.
	{
		AddActorWorldOffset(FlatDirection * MoveSpeed * DeltaTime, true); // Sweep so the threat stops at solid walls instead of passing through.
		SetActorRotation(FlatDirection.Rotation()); // Face the way we are walking.
	}
}

// Called by projectiles. The threat is removed once its health runs out.
void AThreatPawn::ApplyHitDamage(float Amount)
{
	Health -= Amount; // Lose health.
	if (Health <= 0.f) // Dead?
	{
		if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
		{
			GameMode->OnThreatKilled(this); // Let it count the kill and show a burst.
		}
		Destroy(); // Remove the threat from the world.
	}
}

// Damages the wall (0 for false positives) and removes itself.
void AThreatPawn::OnReachedTargetWall()
{
	if (TargetWall) // Only if we still have a wall.
	{
		TargetWall->TakeThreatDamage(ContactDamage); // Hurt the wall (zero for decoys).
	}
	Destroy(); // The threat is used up once it arrives.
}
