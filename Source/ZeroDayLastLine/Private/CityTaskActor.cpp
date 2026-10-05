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
#include "ZeroDayCharacter.h" // The player, whose audio component plays station sounds.
#include "Audio/ZeroDayAudioComponent.h" // The sound effect player.

// Builds the station: a solid mesh plus the shared decay and timer components.
ACityTaskActor::ACityTaskActor()
{
	PrimaryActorTick.bCanEverTick = false; // The components do the timing, not the actor.

	TaskRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TaskRoot")); // An invisible root with no scale of its own.
	RootComponent = TaskRoot; // Everything else attaches to it.

	TaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaskMesh")); // The solid station body.
	TaskMesh->SetupAttachment(TaskRoot); // Attach it to the root.

	// Blockout mesh: a chest-high crate/console stand-in for whichever task this is.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset( // Find the engine cube once.
		TEXT("/Engine/BasicShapes/Cube.Cube") // The engine's built-in 100 unit cube.
	);

	if (CubeMeshAsset.Succeeded()) // Only use it if the engine asset was found.
	{
		TaskMesh->SetStaticMesh(CubeMeshAsset.Object); // Show the cube.
	}

	TaskMesh->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.6f) * StationScale); // 130 wide and 60 tall at the original size (about the size of the artwork), times StationScale.
	TaskMesh->SetCollisionResponseToAllChannels(ECR_Block); // Solid: the player walks around it.

	TaskSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TaskSprite")); // The station artwork.
	TaskSprite->SetupAttachment(TaskRoot); // Attach it to the root.

	StationSprites.SetNum(3); // Room for Power, Food and Medic.

	static ConstructorHelpers::FObjectFinder<UPaperSprite> PowerFinder( // Generator artwork.
		TEXT("/Game/Sprites/S_station_power.S_station_power") // Where the generator sprite lives.
	);

	static ConstructorHelpers::FObjectFinder<UPaperSprite> FoodFinder( // Food store artwork.
		TEXT("/Game/Sprites/S_station_food.S_station_food") // Where the food store sprite lives.
	);

	static ConstructorHelpers::FObjectFinder<UPaperSprite> MedicFinder( // Medical tent artwork.
		TEXT("/Game/Sprites/S_station_medical.S_station_medical") // Where the medical tent sprite lives.
	);

	if (PowerFinder.Succeeded()) // Remember each artwork that exists.
	{
		StationSprites[static_cast<int32>(ECityTaskKind::Power)] = // The slot for the generator.
			PowerFinder.Object; // Its sprite.
	}

	if (FoodFinder.Succeeded()) // Same for the food store.
	{
		StationSprites[static_cast<int32>(ECityTaskKind::Food)] = // The slot for the food store.
			FoodFinder.Object; // Its sprite.
	}

	if (MedicFinder.Succeeded()) // Same for the medical tent.
	{
		StationSprites[static_cast<int32>(ECityTaskKind::Medic)] = // The slot for the medical tent.
			MedicFinder.Object; // Its sprite.
	}

	TaskSprite->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.85f) * StationScale); // Slightly smaller than the 160 unit artwork so it matches the body, times StationScale.

	TaskComp = CreateDefaultSubobject<UCityTaskComponent>(TEXT("TaskComp")); // The decaying condition meter.
	GraceTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("GraceTimer")); // The shared countdown timer.
}

// Wire the decay meter to the grace timer: critical starts it, attending cancels it, expiry fails the task.
void ACityTaskActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	UpdateSprite(); // Show the right artwork for whichever station this is.

	TaskComp->OnBecameCritical.AddDynamic( // Meter critical -> start the countdown.
		this, // The object to call back.
		&ACityTaskActor::HandleBecameCritical // The function to call.
	);

	TaskComp->OnAttended.AddDynamic( // Attended -> stop the countdown.
		this, // The object to call back.
		&ACityTaskActor::HandleAttended // The function to call.
	);

	GraceTimer->OnExpired.AddDynamic( // Countdown over -> the task fails.
		this, // The object to call back.
		&ACityTaskActor::HandleGraceExpired // The function to call.
	);
}

// Chooses which station this is and shows its artwork.
void ACityTaskActor::SetTaskKind(ECityTaskKind NewKind)
{
	TaskKind = NewKind; // Remember which station this is.
	UpdateSprite(); // Show its artwork.
}

// Picks the artwork for the current TaskKind and hides the blockout body if it has one.
void ACityTaskActor::UpdateSprite()
{
	UPaperSprite* Wanted = // The artwork for this kind, if it exists.
		StationSprites.IsValidIndex(static_cast<int32>(TaskKind)) // Is the kind a valid slot?
		? StationSprites[static_cast<int32>(TaskKind)].Get() // Yes: use its sprite.
		: nullptr; // No: nothing to show.

	if (Wanted) // We have artwork.
	{
		ZeroDaySprites::ConfigureFlat( // Lay it flat and upright.
			TaskSprite, // The component that draws it.
			Wanted, // The picture.
			ZeroDaySprites::UprightRotation(), // Art stays upright on screen.
			8.f // Lift it slightly above the ground.
		);

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

	if (AZeroDayGameMode* GameMode = // Find our game mode.
		Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		const EResourceType Cost = ResourceForTask(TaskKind); // Food store -> Food, generator -> Power, tent -> Medical.

		if (!GameMode->SpendResource(Cost, 1)) // Try to pay one.
		{
			GameMode->PostAlert( // Say what is missing.
				FString::Printf( // Build the message.
					TEXT("The %s station needs %s. Collect some first."), // The message text.
					TaskKindToString(TaskKind), // Which station.
					ResourceTypeToString(Cost) // Which resource it needs.
				),
				FColor::Orange // Warning colour.
			);

			return; // Not enough resources.
		}

		GameMode->PostEffect( // Green ring shows the survivors were helped.
			GetActorLocation(), // At the station.
			FLinearColor::Green, // Success colour.
			150.f, // Ring size.
			0.6f // Ring duration.
		);
	}

	TaskComp->Attend(); // Paid up, so restore the station.

	// Play station attended audio after the station has been successfully attended.
	if (AZeroDayCharacter* Character = // The player, who owns the audio component.
		Cast<AZeroDayCharacter>(
			UGameplayStatics::GetPlayerPawn(this, 0))) // Player 0's pawn.
	{
		if (UZeroDayAudioComponent* AudioComponent = // The sound player.
			Character->GetAudioComponent())
		{
			AudioComponent->PlayStationAttended( // Confirm the attend with a sound.
				GetActorLocation() // From the station.
			);
		}
	}
}

// Current condition (0-100), for the HUD.
float ACityTaskActor::GetCondition() const
{
	return TaskComp->GetCondition(); // Read the meter.
}

// Half the station's width (65 units at the original size), scaled: how far from its centre the player can stand and still reach it.
float ACityTaskActor::GetInteractRadius() const
{
	return 65.f * StationScale; // 195 units at 3x.
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
	GraceTimer->StartCooldown( // Start the one-shot countdown.
		GracePeriodSeconds, // How long the player has.
		false // Do not loop.
	);
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

		// Play the station failed sound when the player runs out of time.
		if (AZeroDayCharacter* Character = // The player, who owns the audio component.
			Cast<AZeroDayCharacter>(
				UGameplayStatics::GetPlayerPawn(this, 0))) // Player 0's pawn.
		{
			if (UZeroDayAudioComponent* AudioComponent = // The sound player.
				Character->GetAudioComponent())
			{
				AudioComponent->PlayStationFailed( // Tell the player a survivor was lost.
					GetActorLocation() // From the station.
				);
			}
		}

		TaskComp->RestoreCondition(50.f); // Give the station a partial second chance instead of leaving it dead.
	}
}
