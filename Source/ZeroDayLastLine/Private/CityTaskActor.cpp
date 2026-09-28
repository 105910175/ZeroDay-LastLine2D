#include "CityTaskActor.h" // This class's own declaration.
#include "CityTaskComponent.h" // The decaying meter.
#include "CooldownComponent.h" // The grace timer.
#include "ZeroDayGameMode.h" // Where resources are spent.
#include "Components/StaticMeshComponent.h" // The solid station body.
#include "Components/SceneComponent.h" // The invisible root.
#include "ZeroDaySprites.h" // Helper that lays sprites flat.
#include "PaperSprite.h" // The artwork asset type.
#include "PaperSpriteComponent.h" // Draws the artwork.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load engine assets.

// Builds the station: a solid mesh plus the shared decay and timer components.
ACityTaskActor::ACityTaskActor()
{
	PrimaryActorTick.bCanEverTick = false; // The components do the timing, not the actor.

	TaskRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TaskRoot")); // An invisible root with no scale of its own.
	RootComponent = TaskRoot; // Everything else attaches to it.

	TaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaskMesh")); // The solid station body.
	TaskMesh->SetupAttachment(TaskRoot); // Attach it to the root.

	// Blockout mesh: a chest-high crate/console stand-in for whichever task this is.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube")); // Find the engine cube once.
	if (CubeMeshAsset.Succeeded()) // Only use it if it was found.
	{
		TaskMesh->SetStaticMesh(CubeMeshAsset.Object); // Show the cube.
	}
	TaskMesh->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.6f)); // 130 wide and 60 tall: about the size of the artwork.

	TaskMesh->SetCollisionResponseToAllChannels(ECR_Block); // Solid: the player walks around it.

	TaskSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TaskSprite")); // The station artwork.
	TaskSprite->SetupAttachment(TaskRoot); // Attach it to the root.
	StationSprites.SetNum(3); // Power, Food, Medic.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> PowerFinder(TEXT("/Game/Sprites/S_station_power.S_station_power")); // Generator artwork.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> FoodFinder(TEXT("/Game/Sprites/S_station_food.S_station_food")); // Food store artwork.
	static ConstructorHelpers::FObjectFinder<UPaperSprite> MedicFinder(TEXT("/Game/Sprites/S_station_medical.S_station_medical")); // Medical tent artwork.
	if (PowerFinder.Succeeded()) { StationSprites[static_cast<int32>(ECityTaskKind::Power)] = PowerFinder.Object; } // Remember each one that exists.
	if (FoodFinder.Succeeded()) { StationSprites[static_cast<int32>(ECityTaskKind::Food)] = FoodFinder.Object; }
	if (MedicFinder.Succeeded()) { StationSprites[static_cast<int32>(ECityTaskKind::Medic)] = MedicFinder.Object; }
	TaskSprite->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.85f)); // Slightly smaller than the 160 unit artwork so it matches the body.

	TaskComp = CreateDefaultSubobject<UCityTaskComponent>(TEXT("TaskComp")); // The decaying condition meter.
	GraceTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("GraceTimer")); // The shared countdown timer.
}

// Wire the decay meter to the grace timer: critical starts it, attending cancels it, expiry fails the task.
void ACityTaskActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	UpdateSprite(); // Show the right artwork for whichever station this is.
	TaskComp->OnBecameCritical.AddDynamic(this, &ACityTaskActor::HandleBecameCritical); // Meter critical -> start the countdown.
	TaskComp->OnAttended.AddDynamic(this, &ACityTaskActor::HandleAttended); // Attended -> stop the countdown.
	GraceTimer->OnExpired.AddDynamic(this, &ACityTaskActor::HandleGraceExpired); // Countdown over -> the task fails.
}

// Chooses which station this is (used when the game builds the city) and shows its artwork.
void ACityTaskActor::SetTaskKind(ECityTaskKind NewKind)
{
	TaskKind = NewKind; // Remember which station this is.
	UpdateSprite(); // Show its artwork.
}

// Picks the artwork for the current TaskKind and hides the blockout body if it has one.
void ACityTaskActor::UpdateSprite()
{
	UPaperSprite* Wanted = StationSprites.IsValidIndex(static_cast<int32>(TaskKind)) ? StationSprites[static_cast<int32>(TaskKind)].Get() : nullptr; // The artwork for this kind, if it exists.
	if (Wanted) // We have artwork.
	{
		ZeroDaySprites::ConfigureFlat(TaskSprite, Wanted, ZeroDaySprites::UprightRotation(), 8.f); // Lay it flat and upright.
		TaskMesh->SetVisibility(false); // Hide the blockout cube but keep its collision.
	}
}

// Attending costs one of this station's resource. If the player can't pay, nothing happens.
void ACityTaskActor::Attend()
{
	if (!bIsActiveThisLevel) // Not in play this level.
	{
		return; // Ignore the request.
	}

	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
	{
		const EResourceType Cost = ResourceForTask(TaskKind); // Food store -> Food, generator -> Power, tent -> Medical.
		if (!GameMode->SpendResource(Cost, 1)) // Try to pay one.
		{
			GameMode->PostAlert(FString::Printf(TEXT("The %s station needs %s. Collect some first."), TaskKindToString(TaskKind), ResourceTypeToString(Cost)), FColor::Orange); // Say what is missing.
			return; // Not enough resources.
		}
		GameMode->PostEffect(GetActorLocation(), FLinearColor::Green, 150.f, 0.6f); // Green ring shows the survivors were helped.
	}

	TaskComp->Attend(); // Paid up, so restore the station.
}

// Current condition (0-100), for the HUD.
float ACityTaskActor::GetCondition() const
{
	return TaskComp->GetCondition(); // Read the meter.
}

// Inactive stations are hidden and stop decaying.
void ACityTaskActor::SetActiveThisLevel(bool bActive)
{
	bIsActiveThisLevel = bActive; // Remember the state for other systems.
	SetActorHiddenInGame(!bActive); // Hide the station when it isn't in play.
	TaskComp->SetComponentTickEnabled(bActive); // Only decay while in play.
}

// Puts the station back to a clean state for a fresh level.
void ACityTaskActor::ResetForLevel(float DecayScale)
{
	GraceTimer->CancelCooldown(); // No leftover countdown.
	TaskComp->ResetTask(DecayScale); // Full meter at the new level's decay speed.
}

// Condition hit the threshold: the player now has GracePeriodSeconds to respond.
void ACityTaskActor::HandleBecameCritical()
{
	GraceTimer->StartCooldown(GracePeriodSeconds, false); // Start the one-shot countdown.
}

// Player responded in time, so the grace period is cancelled.
void ACityTaskActor::HandleAttended()
{
	GraceTimer->CancelCooldown(); // Stop the countdown.
}

// Ran out of time while still critical: the task has failed and a survivor is lost.
void ACityTaskActor::HandleGraceExpired()
{
	if (TaskComp->IsCritical()) // Still critical when the countdown ended?
	{
		OnTaskFailed.Broadcast(this); // Tell the game mode (it removes a survivor).
		TaskComp->RestoreCondition(50.f); // Give the station a partial second chance instead of leaving it dead.
	}
}
