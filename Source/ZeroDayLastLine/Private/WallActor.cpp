#include "WallActor.h"
#include "BreachableComponent.h"
#include "CooldownComponent.h"
#include "ThreatPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"

// Builds the wall: a solid mesh, a trigger zone for arriving threats, and the shared health and timer components.
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

// Bind here rather than the constructor so the components are fully initialised.
void AWallActor::BeginPlay()
{
	Super::BeginPlay();

	Breachable->OnStatusChanged.AddDynamic(this, &AWallActor::HandleStatusChanged);
	RemediationTimer->OnExpired.AddDynamic(this, &AWallActor::HandleRemediationExpired);
	ThreatContactZone->OnComponentBeginOverlap.AddDynamic(this, &AWallActor::HandleThreatContactBegin);
}

// Damage only counts while the wall is in play this level.
void AWallActor::TakeThreatDamage(float Amount)
{
	if (!bIsActiveThisLevel)
	{
		return;
	}
	Breachable->ApplyDamage(Amount);
}

// Only a breached wall can be fixed; Safe or Threatened walls are left alone.
void AWallActor::AttemptFix()
{
	if (Breachable->GetStatus() == EWallStatus::Breached)
	{
		Breachable->FullyRepair();
	}
}

// Convenience passthrough to the Breachable component.
EWallStatus AWallActor::GetStatus() const
{
	return Breachable->GetStatus();
}

// Inactive walls are hidden and have collision off so they can't be hit or block anything.
void AWallActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive;
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
}

// Health as a 0-1 fraction, for the HUD.
float AWallActor::GetHealthPercent() const
{
	return Breachable->GetHealthPercent();
}

// Seconds left to fix a breach (0 when not breached), for the HUD.
float AWallActor::GetRemediationTimeRemaining() const
{
	return RemediationTimer->GetTimeRemaining();
}

// Threats register themselves so Insta Scan and Validate can find them per wall.
void AWallActor::RegisterInboundThreat(AThreatPawn* Threat)
{
	InboundThreats.AddUnique(Threat);
}

// Called when a threat is destroyed, whether shot or having hit the wall.
void AWallActor::UnregisterInboundThreat(AThreatPawn* Threat)
{
	InboundThreats.Remove(Threat);
}

// Counts only threats that are still alive.
int32 AWallActor::GetInboundThreatCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats)
	{
		if (Threat.IsValid())
		{
			++Count;
		}
	}
	return Count;
}

// How many inbound threats an Insta Scan has flagged.
int32 AWallActor::GetDetectedThreatCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats)
	{
		if (Threat.IsValid() && Threat->IsDetected())
		{
			++Count;
		}
	}
	return Count;
}

// Detected threats that Validate hasn't checked yet.
int32 AWallActor::GetUnvalidatedDetectedCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats)
	{
		if (Threat.IsValid() && Threat->IsDetected() && !Threat->IsValidated())
		{
			++Count;
		}
	}
	return Count;
}

// Insta Scan: flags everything inbound without revealing what it is.
int32 AWallActor::MarkInboundDetected()
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats)
	{
		if (Threat.IsValid())
		{
			Threat->MarkDetected();
			++Count;
		}
	}
	return Count;
}

// Validate: reveals real vs false positive and tracks the highest real severity.
void AWallActor::ValidateInbound(int32& OutReal, int32& OutFalsePositives, EThreatSeverity& OutHighestSeverity)
{
	OutReal = 0;
	OutFalsePositives = 0;
	OutHighestSeverity = EThreatSeverity::Low;

	for (const TWeakObjectPtr<AThreatPawn>& Threat : InboundThreats)
	{
		if (!Threat.IsValid() || !Threat->IsDetected())
		{
			continue;
		}

		Threat->MarkValidated();
		if (Threat->bIsDecoy)
		{
			++OutFalsePositives;
		}
		else
		{
			++OutReal;
			// Enum order is Low < Medium < Critical, so a numeric compare finds the worst.
			if (static_cast<uint8>(Threat->Severity) > static_cast<uint8>(OutHighestSeverity))
			{
				OutHighestSeverity = Threat->Severity;
			}
		}
	}
}

// Announce every change, then start or stop the breach countdown.
void AWallActor::HandleStatusChanged(EWallStatus NewStatus)
{
	OnStatusUpdated.Broadcast(this, NewStatus);

	if (NewStatus == EWallStatus::Breached)
	{
		RemediationTimer->StartCooldown(RemediationWindowSeconds, false);
	}
	else
	{
		RemediationTimer->CancelCooldown();
	}
}

// Time ran out. Only counts as lost if the wall is still breached.
void AWallActor::HandleRemediationExpired()
{
	if (Breachable->GetStatus() == EWallStatus::Breached)
	{
		OnWallLost.Broadcast(this);
	}
}

// A threat reached the wall; it applies its own damage and removes itself.
void AWallActor::HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AThreatPawn* Threat = Cast<AThreatPawn>(OtherActor))
	{
		Threat->OnReachedTargetWall();
	}
}
