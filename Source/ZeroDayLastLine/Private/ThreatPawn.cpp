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

#include "ZeroDayCharacter.h" // The player, whose audio component plays the death sound.
#include "Audio/ZeroDayAudioComponent.h" // The sound effect player.

// Threat channel (GameTraceChannel2). Overlaps most things so shots can hit it, but blocks on wall meshes.
AThreatPawn::AThreatPawn()
{
	PrimaryActorTick.bCanEverTick = true; // Threats move every frame.

	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere")); // Create the collision body.
	RootComponent = HitSphere; // The sphere is the root that everything attaches to.
	HitSphere->SetSphereRadius(25.f); // Twenty five units is the real body size: the 128 pixel sprite is drawn only about 50 units wide (2.56 pixels per unit), so half of that.
	HitSphere->SetCollisionObjectType(ECC_GameTraceChannel2); // Report as the Threat channel.
	HitSphere->SetCollisionResponseToAllChannels(ECR_Overlap); // Overlap most things so shots can hit it.
	HitSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // But stop physically at wall meshes.

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh")); // Create the visible body.
	BodyMesh->SetupAttachment(RootComponent); // Attach it to the collision sphere.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // The mesh is only for looks.

	// Blockout mesh: an upright cylinder reads as a humanoid silhouette better than a sphere.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset( // Find the engine cylinder once.
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder") // The engine's built-in cylinder.
	);

	if (CylinderMeshAsset.Succeeded()) // Only use it if it was found.
	{
		BodyMesh->SetStaticMesh(CylinderMeshAsset.Object); // Show the cylinder.
	}

	BodyMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.0f)); // Thin and tall.
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -50.f)); // Sit it so its base is at ground level.

	BodySprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodySprite")); // The zombie artwork.
	BodySprite->SetupAttachment(RootComponent); // It moves and turns with the threat.

	static ConstructorHelpers::FObjectFinder<UPaperSprite> SpriteFinder( // Find the artwork once.
		TEXT("/Game/Sprites/S_threat.S_threat") // Where the threat sprite lives.
	);

	if (SpriteFinder.Succeeded()) // If it exists...
	{
		ZeroDaySprites::ConfigureFlat( // ...lay it flat, facing the walking direction...
			BodySprite, // The component that draws it.
			SpriteFinder.Object, // The picture.
			ZeroDaySprites::HeadingRotation(), // Art turns with the threat.
			4.f // Lift it slightly above the ground.
		);

		BodyMesh->SetVisibility(false); // ...and hide the blockout cylinder.
	}
}

// Nothing extra to do when the threat appears.
void AThreatPawn::BeginPlay()
{
	Super::BeginPlay(); // Let the base class finish starting up.

	HitSphere->SetPhysMaterialOverride( // Slippery, non-bouncy body.
		ZeroDayPhysics::Get(EZeroDayMaterial::Threat) // The threat's physics material.
	);

	// Make the threat bigger: the artwork, the blockout body and the collision sphere all grow by the
	// same factor, so shots and walls react to what the player actually sees.
	HitSphere->SetSphereRadius(HitSphere->GetUnscaledSphereRadius() * VisualScale); // A bigger collision body (25 -> 100 at 4x), matching the bigger picture.
	BodySprite->SetRelativeScale3D(FVector(VisualScale)); // A bigger picture.
	BodyMesh->SetRelativeScale3D(BodyMesh->GetRelativeScale3D() * VisualScale); // A bigger blockout cylinder (only shown if the art is missing).
}

// Radius of the collision body in world units, so the HUD can size its markers to match.
float AThreatPawn::GetBodyRadius() const
{
	return HitSphere ? HitSphere->GetScaledSphereRadius() : 50.f; // Fall back to the original size.
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
void AThreatPawn::InitialiseThreat(
	AWallActor* InTargetWall, // The wall to attack.
	EThreatSeverity InSeverity, // How dangerous this threat is.
	bool bInIsDecoy, // True for a harmless false positive.
	float SpeedScale, // The level's speed multiplier.
	float HealthScale) // The level's health multiplier.
{
	TargetWall = InTargetWall; // Remember which wall to attack.
	Severity = InSeverity; // Remember how dangerous this threat is.
	bIsDecoy = bInIsDecoy; // Remember whether it is a harmless false positive.

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

	const FVector ToTarget = // Vector from us to the wall.
		TargetWall->GetActorLocation() - GetActorLocation();

	const FVector FlatDirection = FVector( // Ignore height so threats stay on the ground plane.
		ToTarget.X, // Keep the X part.
		ToTarget.Y, // Keep the Y part.
		0.f // Drop the height.
	).GetSafeNormal(); // Reduce it to a pure direction.

	if (!FlatDirection.IsNearlyZero()) // Only move if we have a direction.
	{
		AddActorWorldOffset( // Sweep so the threat stops at solid walls instead of passing through.
			FlatDirection * MoveSpeed * DeltaTime, // Distance this frame = direction x speed x time.
			true // Check for collisions while moving.
		);

		SetActorRotation(FlatDirection.Rotation()); // Face the way we are walking.
	}
}

// Called by projectiles. The threat is removed once its health runs out.
void AThreatPawn::ApplyHitDamage(float Amount)
{
	Health -= Amount; // Lose health.

	if (Health <= 0.f) // Dead?
	{
		if (AZeroDayGameMode* GameMode = // Find our game mode.
			Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GameMode->OnThreatKilled(this); // Let it count the kill and show a burst.
		}

		// Play the threat destroyed sound.
		if (AZeroDayCharacter* Character = // The player who caused this threat.
			Cast<AZeroDayCharacter>(GetInstigator()))
		{
			if (UZeroDayAudioComponent* AudioComponent = // The sound player.
				Character->GetAudioComponent())
			{
				AudioComponent->PlayThreatDestroyed( // Play the death sound.
					GetActorLocation() // Where the threat died.
				);
			}
		}

		Destroy(); // Remove the threat.
	}
}

// Damages the wall and removes the threat.
void AThreatPawn::OnReachedTargetWall()
{
	if (TargetWall) // Only if we still have a wall.
	{
		TargetWall->TakeThreatDamage(ContactDamage); // Hurt it (0 for false positives).
	}

	Destroy(); // The threat is spent either way.
}
