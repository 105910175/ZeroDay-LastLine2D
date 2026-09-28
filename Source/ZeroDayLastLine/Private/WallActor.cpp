#include "WallActor.h" // This class's own declaration.
#include "BreachableComponent.h" // Health and status logic.
#include "CooldownComponent.h" // Breach countdown timer.
#include "ThreatPawn.h" // So we can talk to arriving threats.
#include "ZeroDayGameMode.h" // Where Patch kits are spent and stats are kept.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "Components/StaticMeshComponent.h" // The wall body.
#include "Components/BoxComponent.h" // The threat trigger volume.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine assets.

// Builds the wall: a solid mesh, a trigger zone for arriving threats, and the shared health and timer components.
AWallActor::AWallActor()
{
	PrimaryActorTick.bCanEverTick = false; // The wall reacts to events, so it never needs a per-frame tick.

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh")); // Create the visible, solid body.
	RootComponent = WallMesh; // Make the mesh the actor's root so everything else attaches to it.

	// Blockout mesh so the wall is visible before real art exists: a 100 unit engine cube stretched into a low rampart.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube")); // Find the engine cube once.
	if (CubeMeshAsset.Succeeded()) // Only use it if the engine asset was found.
	{
		WallMesh->SetStaticMesh(CubeMeshAsset.Object); // Show the cube.
	}
	WallMesh->SetRelativeScale3D(FVector(3.0f, 0.5f, 0.6f)); // 300 long, 50 deep, 60 tall: low enough not to hide the map behind it.

	WallMesh->SetCollisionObjectType(ECC_GameTraceChannel1); // Wall channel (GameTraceChannel1).
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block); // Block everything by default...
	WallMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap); // ...but let projectiles pass so the player can shoot threats approaching from outside.

	ThreatContactZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ThreatContactZone")); // Trigger volume around the wall.
	ThreatContactZone->SetupAttachment(RootComponent); // Attach it to the wall so it moves with it.
	// The zone inherits the mesh's 3 x 0.5 x 0.6 scale, so divide to get roughly 170 x 120 x 120 world half-extents.
	ThreatContactZone->SetBoxExtent(FVector(57.f, 240.f, 200.f)); // Slightly longer and deeper than the wall itself.
	ThreatContactZone->SetCollisionObjectType(ECC_GameTraceChannel1); // Reports as the Wall channel.
	ThreatContactZone->SetCollisionResponseToAllChannels(ECR_Ignore); // Ignore everything...
	ThreatContactZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap); // ...except threats, which should trigger it.

	Breachable = CreateDefaultSubobject<UBreachableComponent>(TEXT("Breachable")); // Health and status tracking.
	RemediationTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("RemediationTimer")); // Countdown used after a breach.
}

// Bind here rather than the constructor so the components are fully initialised.
void AWallActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class do its setup first.

	Breachable->OnStatusChanged.AddDynamic(this, &AWallActor::HandleStatusChanged); // Tell us when health changes the status.
	RemediationTimer->OnExpired.AddDynamic(this, &AWallActor::HandleRemediationExpired); // Tell us when the breach countdown ends.
	ThreatContactZone->OnComponentBeginOverlap.AddDynamic(this, &AWallActor::HandleThreatContactBegin); // Tell us when something arrives.
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
	SetActorHiddenInGame(!bActive); // Hide the wall when it is not in play.
	SetActorEnableCollision(bActive); // Turn its collision off when it is not in play.
}

// Health as a 0-1 fraction, for the HUD.
float AWallActor::GetHealthPercent() const
{
	return Breachable->GetHealthPercent(); // Ask the health component.
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

// Announce every change, then start or stop the breach countdown.
void AWallActor::HandleStatusChanged(EWallStatus NewStatus)
{
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
