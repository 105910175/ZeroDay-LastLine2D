#include "WallActor.h" // This class's own declaration.
#include "BreachableComponent.h" // Health and status logic.
#include "CooldownComponent.h" // Breach countdown timer.
#include "ThreatPawn.h" // So we can talk to arriving threats.
#include "ZeroDayGameMode.h" // Where Patch kits are spent and stats are kept.
#include "ZeroDayPhysics.h" // The wall's physics material.
#include "ZeroDaySprites.h" // Helper that lays sprites flat.
#include "PaperSprite.h" // The wall artwork asset type.
#include "PaperSpriteComponent.h" // Draws one tile of artwork.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "Components/SceneComponent.h" // The invisible root.
#include "Components/StaticMeshComponent.h" // The wall body.
#include "Components/BoxComponent.h" // The threat trigger volume.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine and content assets.

namespace // Only this file uses these.
{
	constexpr float TileArtWidth = 256.f; // Each artwork tile is 256 pixels (= units) wide.
	constexpr float WallBodyHeight = 60.f; // How tall the invisible collision body is.
	constexpr float WallBodyDepth = 96.f; // How thick the invisible collision body is (matches the 104px-tall artwork with its reinforced footing, so the wall reads as a solid barrier next to the player).
	constexpr float ContactZoneHalfDepth = 160.f; // How far in front of/behind the wall a threat counts as arriving.
}

// Builds the wall: a solid body, a trigger zone for arriving threats, the artwork and the shared components.
AWallActor::AWallActor()
{
	PrimaryActorTick.bCanEverTick = false; // The wall reacts to events, so it never needs a per-frame tick.

	WallRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WallRoot")); // An invisible root with no scale of its own.
	RootComponent = WallRoot; // Everything else attaches to it.

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh")); // Create the solid body.
	WallMesh->SetupAttachment(WallRoot); // Attach it to the root.

	// Blockout mesh: a 100 unit engine cube stretched into a low rampart. It is what blocks movement,
	// and it is what you see if the artwork is missing.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube")); // Find the engine cube once.
	if (CubeMeshAsset.Succeeded()) // Only use it if the engine asset was found.
	{
		WallMesh->SetStaticMesh(CubeMeshAsset.Object); // Show the cube.
	}
	WallMesh->SetRelativeScale3D(FVector(WallLength / 100.f, WallBodyDepth / 100.f, WallBodyHeight / 100.f)); // Long, thin and low.

	WallMesh->SetCollisionObjectType(ECC_GameTraceChannel1); // Wall channel (GameTraceChannel1).
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block); // Block everything by default...
	WallMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap); // ...but let projectiles pass so the player can shoot threats approaching from outside.

	ThreatContactZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ThreatContactZone")); // Trigger volume around the wall.
	ThreatContactZone->SetupAttachment(WallRoot); // Attach it to the root (so the mesh scale does not stretch it).
	ThreatContactZone->SetBoxExtent(FVector(WallLength * 0.5f + 20.f, ContactZoneHalfDepth, 120.f)); // A little longer than the wall and deeper than its body.
	ThreatContactZone->SetCollisionObjectType(ECC_GameTraceChannel1); // Reports as the Wall channel.
	ThreatContactZone->SetCollisionResponseToAllChannels(ECR_Ignore); // Ignore everything...
	ThreatContactZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap); // ...except threats, which should trigger it.

	Breachable = CreateDefaultSubobject<UBreachableComponent>(TEXT("Breachable")); // Health and status tracking.
	RemediationTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("RemediationTimer")); // Countdown used after a breach.

	// The wall artwork for each condition. If any is missing the blockout mesh is shown instead.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> IntactFinder(TEXT("/Game/Sprites/S_wall_intact.S_wall_intact")); // Healthy wall.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> DamagedFinder(TEXT("/Game/Sprites/S_wall_damaged.S_wall_damaged")); // Cracked wall.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> BreachedFinder(TEXT("/Game/Sprites/S_wall_breached.S_wall_breached")); // Broken wall.
	if (IntactFinder.Succeeded() && DamagedFinder.Succeeded() && BreachedFinder.Succeeded()) // All three artworks exist.
	{
		IntactSprite = IntactFinder.Object; // Remember them.
		DamagedSprite = DamagedFinder.Object;
		BreachedSprite = BreachedFinder.Object;
		bHasSprites = true; // We will draw sprite tiles.
		WallMesh->SetVisibility(false); // Hide the blockout body but keep its collision.
	}

	// A fixed pool of tile components, created here so they exist on the CDO like every other
	// sprite in the game. RebuildVisuals() only shows/hides and repositions them; it never
	// creates or destroys components at runtime.
	SpriteTiles.Reserve(MaxWallTiles); // Avoid reallocating as we add them below.
	for (int32 Index = 0; Index < MaxWallTiles; ++Index) // Enough tiles for the longest wall we build.
	{
		UPaperSpriteComponent* Tile = CreateDefaultSubobject<UPaperSpriteComponent>(*FString::Printf(TEXT("SpriteTile%d"), Index)); // Each needs a unique name.
		Tile->SetupAttachment(WallRoot); // Attach it to the root.
		Tile->SetVisibility(false); // Hidden until RebuildVisuals sizes the wall and shows however many tiles it needs.
		SpriteTiles.Add(Tile); // Keep track of it.
	}
}

// Bind here rather than the constructor so the components are fully initialised.
void AWallActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class do its setup first.

	Breachable->OnStatusChanged.AddDynamic(this, &AWallActor::HandleStatusChanged); // Tell us when health changes the status.
	RemediationTimer->OnExpired.AddDynamic(this, &AWallActor::HandleRemediationExpired); // Tell us when the breach countdown ends.
	ThreatContactZone->OnComponentBeginOverlap.AddDynamic(this, &AWallActor::HandleThreatContactBegin); // Tell us when something arrives.

	WallMesh->SetPhysMaterialOverride(ZeroDayPhysics::Get(EZeroDayMaterial::Wall)); // Rough stone: pickups thrown at it barely bounce.
	SetWallLength(WallLength); // Size the body and zone for the configured length (also builds the artwork).
}

// Resizes the body, the trigger zone and the artwork to a new length.
void AWallActor::SetWallLength(float NewLength)
{
	WallLength = FMath::Max(50.f, NewLength); // Never shorter than 50 units.
	WallMesh->SetRelativeScale3D(FVector(WallLength / 100.f, WallBodyDepth / 100.f, WallBodyHeight / 100.f)); // Stretch the body (the cube is 100 units).
	ThreatContactZone->SetBoxExtent(FVector(WallLength * 0.5f + 20.f, ContactZoneHalfDepth, 120.f)); // Keep the trigger zone matched.
	RebuildVisuals(); // Redraw the artwork at the new length.
}

// Sizes the row of artwork tiles to exactly cover the wall's length, by showing and positioning
// however many of the pre-built pool are needed and hiding the rest.
void AWallActor::RebuildVisuals()
{
	if (!bHasSprites) // No artwork available.
	{
		for (UPaperSpriteComponent* Tile : SpriteTiles) // Make sure none of the pool is left showing.
		{
			if (Tile) // Skip any that are somehow gone.
			{
				Tile->SetVisibility(false); // The blockout mesh is visible instead.
			}
		}
		return; // Nothing more to do.
	}

	const int32 TileCount = FMath::Clamp(FMath::RoundToInt(WallLength / TileArtWidth), 1, SpriteTiles.Num()); // How many tiles fit, capped to the pool size.
	const float TileLength = WallLength / TileCount; // Stretch each slightly so the row ends exactly at the wall's ends.
	for (int32 Index = 0; Index < SpriteTiles.Num(); ++Index) // Every tile in the pool.
	{
		UPaperSpriteComponent* Tile = SpriteTiles[Index]; // This slot.
		if (!Tile) // Skip any that are somehow gone.
		{
			continue; // Next slot.
		}
		if (Index < TileCount) // This one is needed for the current length.
		{
			ZeroDaySprites::ConfigureFlat(Tile, IntactSprite, ZeroDaySprites::HeadingRotation(), 6.f); // Lay it flat with the wall's long axis as the art's width.
			Tile->SetRelativeLocation(FVector(-WallLength * 0.5f + (Index + 0.5f) * TileLength, 0.f, 6.f)); // Place it along the wall.
			Tile->SetRelativeScale3D(FVector(TileLength / TileArtWidth, 1.f, 1.f)); // Stretch it to fit its share of the length.
			Tile->SetVisibility(true); // Show it.
		}
		else // Not needed at this length.
		{
			Tile->SetVisibility(false); // Hide the rest of the pool.
		}
	}
	UpdateVisuals(); // Show the artwork that matches the current condition.
}

// Switches every tile to the artwork for the wall's current condition.
void AWallActor::UpdateVisuals()
{
	UPaperSprite* Wanted = IntactSprite; // Default to the healthy artwork.
	switch (Breachable->GetStatus()) // Choose by condition.
	{
	case EWallStatus::Threatened: Wanted = DamagedSprite; break; // Cracked.
	case EWallStatus::Breached: Wanted = BreachedSprite; break; // Broken.
	default: break; // Safe keeps the healthy artwork.
	}
	for (UPaperSpriteComponent* Tile : SpriteTiles) // Update each tile.
	{
		if (Tile) // Skip any that are gone.
		{
			Tile->SetSprite(Wanted); // Swap the picture.
		}
	}
}

// Damage only counts while the wall is in play this level.
void AWallActor::TakeThreatDamage(float Amount)
{
	if (!bIsActiveThisLevel) // Walls not in play ignore damage.
	{
		return; // Nothing to do.
	}
	Breachable->ApplyDamage(Amount); // Pass the damage to the health component.
}

// Only a breached wall can be fixed, and fixing costs one Patch kit.
bool AWallActor::AttemptFix()
{
	if (Breachable->GetStatus() != EWallStatus::Breached) // Is there actually a breach to fix?
	{
		return false; // Nothing was broken, so nothing was fixed.
	}

	AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)); // Find our game mode.
	if (GameMode && !GameMode->SpendResource(EResourceType::Patch, 1)) // Try to pay one Patch kit.
	{
		GameMode->PostAlert(TEXT("Not enough Patch kits to fix the wall. Clear the threats or collect more."), FColor::Orange); // Say why nothing happened.
		return false; // Could not afford it.
	}

	Breachable->FullyRepair(); // Restore full health (this also cancels the countdown via the status event).
	if (GameMode) // If there is a game mode to tell...
	{
		GameMode->RecordWallFixed(); // ...count the fix for the result screen.
	}
	return true; // Tell the caller the fix worked.
}

// Convenience passthrough to the Breachable component.
EWallStatus AWallActor::GetStatus() const
{
	return Breachable->GetStatus(); // Safe, Threatened or Breached.
}

// Inactive walls are hidden and have collision off so they can't be hit or block anything.
void AWallActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive; // Remember the state for other systems to check.
	SetActorHiddenInGame(!bActive); // Hide the wall (and its artwork) when it is not in play.
	SetActorEnableCollision(bActive); // Turn its collision off when it is not in play.
}

// Health as a 0-1 fraction, for the HUD.
float AWallActor::GetHealthPercent() const
{
	return Breachable->GetHealthPercent(); // Ask the health component.
}

// Distance to the nearest point along the wall's length, so a player standing at either end
// of a long wall is still judged as "close to the wall", not just at its midpoint.
float AWallActor::GetDistanceToPoint(const FVector& Point) const
{
	const FVector Origin = GetActorLocation(); // The wall's centre.
	const FVector AxisDirection = GetActorForwardVector().GetSafeNormal2D(); // The wall's long axis (its length runs along local +X).
	const float HalfLength = WallLength * 0.5f; // How far the wall extends either side of its centre.
	const float Projected = FVector::DotProduct(Point - Origin, AxisDirection); // How far along the axis the point sits.
	const float Clamped = FMath::Clamp(Projected, -HalfLength, HalfLength); // Stay within the wall's actual length.
	const FVector NearestPoint = Origin + AxisDirection * Clamped; // The closest point on the wall.
	return FVector::Dist(NearestPoint, Point); // Straight-line distance from there to the given point.
}

// Seconds left to fix a breach (0 when not breached), for the HUD.
float AWallActor::GetRemediationTimeRemaining() const
{
	return RemediationTimer->GetTimeRemaining(); // Read the countdown timer.
}

// A level can shorten or lengthen how long the player has to react to a breach.
void AWallActor::ApplyLevelSettings(float NewRemediationWindowSeconds)
{
	RemediationWindowSeconds = NewRemediationWindowSeconds; // Store it; used the next time the wall is breached.
}

// Puts the wall back to a clean state so a level can be restarted or the next one started.
void AWallActor::ResetForLevel()
{
	InboundThreats.Empty(); // Forget threats from the previous run.
	RemediationTimer->CancelCooldown(); // A breach countdown must not carry over.
	Breachable->FullyRepair(); // Full health again (status returns to Safe).
	UpdateVisuals(); // Make sure the artwork shows the healthy wall.
}

// Threats register themselves so Insta Scan and Validate can find them per wall.
void AWallActor::RegisterInboundThreat(AThreatPawn* Threat)
{
	InboundThreats.AddUnique(Threat); // Add it once; ignore duplicates.
}

// Called when a threat is destroyed, whether shot or having hit the wall.
void AWallActor::UnregisterInboundThreat(AThreatPawn* Threat)
{
	InboundThreats.Remove(Threat); // Drop it from the list.
}

// Counts only threats that are still alive.
int32 AWallActor::GetInboundThreatCount() const
{
	int32 Count = 0; // Running total.
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats) // Look at every registered threat.
	{
		if (Threat.IsValid()) // Skip ones that have been destroyed.
		{
			++Count; // This one is alive.
		}
	}
	return Count; // Number of live inbound threats.
}

// How many inbound threats an Insta Scan has flagged.
int32 AWallActor::GetDetectedThreatCount() const
{
	int32 Count = 0; // Running total.
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats) // Check each inbound threat.
	{
		if (Threat.IsValid() && Threat->IsDetected()) // Alive and flagged by a scan.
		{
			++Count; // Count it.
		}
	}
	return Count; // Number of detected threats.
}

// Detected threats that Validate hasn't checked yet.
int32 AWallActor::GetUnvalidatedDetectedCount() const
{
	int32 Count = 0; // Running total.
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats) // Check each inbound threat.
	{
		if (Threat.IsValid() && Threat->IsDetected() && !Threat->IsValidated()) // Flagged but not yet checked.
		{
			++Count; // Count it.
		}
	}
	return Count; // Number of threats still worth validating.
}

// Insta Scan: flags everything inbound without revealing what it is.
int32 AWallActor::MarkInboundDetected()
{
	int32 Count = 0; // Running total.
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats) // Go through every inbound threat.
	{
		if (Threat.IsValid()) // Only living threats.
		{
			Threat->MarkDetected(); // Flag it so the HUD can mark it.
			++Count; // Count it.
		}
	}
	return Count; // How many contacts the scan found.
}

// Validate: reveals real vs false positive and tracks the highest real severity.
void AWallActor::ValidateInbound(int32& OutReal, int32& OutFalsePositives, EThreatSeverity& OutHighestSeverity)
{
	OutReal = 0; // Reset the outputs.
	OutFalsePositives = 0; // Reset the outputs.
	OutHighestSeverity = EThreatSeverity::Low; // Start at the lowest severity.

	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats) // Check each inbound threat.
	{
		if (!Threat.IsValid() || !Threat->IsDetected()) // Skip destroyed or unscanned threats.
		{
			continue; // Validate only works on scanned threats.
		}

		Threat->MarkValidated(); // Reveal its true nature to the HUD.
		if (Threat->bIsDecoy) // Is it a harmless false positive?
		{
			++OutFalsePositives; // Count it as a false positive.
		}
		else
		{
			++OutReal; // Count it as a real threat.
			if (static_cast<uint8>(Threat->Severity) > static_cast<uint8>(OutHighestSeverity)) // Enum order is Low < Medium < Critical.
			{
				OutHighestSeverity = Threat->Severity; // Remember the worst real threat.
			}
		}
	}
}

// Announce every change, swap the artwork, then start or stop the breach countdown.
void AWallActor::HandleStatusChanged(EWallStatus NewStatus)
{
	UpdateVisuals(); // Show intact, cracked or broken artwork.
	OnStatusUpdated.Broadcast(this, NewStatus); // Let the game mode raise an alert.

	if (NewStatus == EWallStatus::Breached) // A breach starts the clock...
	{
		RemediationTimer->StartCooldown(RemediationWindowSeconds, false); // ...the player must fix it in time.
	}
	else
	{
		RemediationTimer->CancelCooldown(); // Any other status means the danger has passed.
	}
}

// Time ran out. Only counts as lost if the wall is still breached.
void AWallActor::HandleRemediationExpired()
{
	if (Breachable->GetStatus() == EWallStatus::Breached) // Still broken after the countdown?
	{
		OnWallLost.Broadcast(this); // The wall is lost: the game mode ends the level.
	}
}

// A threat reached the wall; it applies its own damage and removes itself.
void AWallActor::HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AThreatPawn* Threat = Cast<AThreatPawn>(OtherActor)) // Is the thing that arrived a threat?
	{
		Threat->OnReachedTargetWall(); // Let it deal its damage and clean itself up.
	}
}
