#include "CityTaskActor.h"
#include "CityTaskComponent.h"
#include "CooldownComponent.h"
#include "ZeroDayGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "ZeroDaySprites.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "ZeroDayCharacter.h"
#include "Audio/ZeroDayAudioComponent.h"

// Builds the station: a solid mesh plus the shared decay and timer components.
ACityTaskActor::ACityTaskActor()
{
    PrimaryActorTick.bCanEverTick = false;

    TaskRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TaskRoot"));
    RootComponent = TaskRoot;

    TaskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaskMesh"));
    TaskMesh->SetupAttachment(TaskRoot);

    // Blockout mesh: a chest-high crate/console stand-in for whichever task this is.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube")
    );

    if (CubeMeshAsset.Succeeded())
    {
        TaskMesh->SetStaticMesh(CubeMeshAsset.Object);
    }

    TaskMesh->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.6f));
    TaskMesh->SetCollisionResponseToAllChannels(ECR_Block);

    TaskSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("TaskSprite"));
    TaskSprite->SetupAttachment(TaskRoot);

    StationSprites.SetNum(3);

    static ConstructorHelpers::FObjectFinder<UPaperSprite> PowerFinder(
        TEXT("/Game/Sprites/S_station_power.S_station_power")
    );

    static ConstructorHelpers::FObjectFinder<UPaperSprite> FoodFinder(
        TEXT("/Game/Sprites/S_station_food.S_station_food")
    );

    static ConstructorHelpers::FObjectFinder<UPaperSprite> MedicFinder(
        TEXT("/Game/Sprites/S_station_medical.S_station_medical")
    );

    if (PowerFinder.Succeeded())
    {
        StationSprites[static_cast<int32>(ECityTaskKind::Power)] =
            PowerFinder.Object;
    }

    if (FoodFinder.Succeeded())
    {
        StationSprites[static_cast<int32>(ECityTaskKind::Food)] =
            FoodFinder.Object;
    }

    if (MedicFinder.Succeeded())
    {
        StationSprites[static_cast<int32>(ECityTaskKind::Medic)] =
            MedicFinder.Object;
    }

    TaskSprite->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.85f));

    TaskComp = CreateDefaultSubobject<UCityTaskComponent>(TEXT("TaskComp"));
    GraceTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("GraceTimer"));
}

// Wire the decay meter to the grace timer: critical starts it, attending cancels it, expiry fails the task.
void ACityTaskActor::BeginPlay()
{
    Super::BeginPlay();

    UpdateSprite();

    TaskComp->OnBecameCritical.AddDynamic(
        this,
        &ACityTaskActor::HandleBecameCritical
    );

    TaskComp->OnAttended.AddDynamic(
        this,
        &ACityTaskActor::HandleAttended
    );

    GraceTimer->OnExpired.AddDynamic(
        this,
        &ACityTaskActor::HandleGraceExpired
    );
}

// Chooses which station this is and shows its artwork.
void ACityTaskActor::SetTaskKind(ECityTaskKind NewKind)
{
    TaskKind = NewKind;
    UpdateSprite();
}

// Picks the artwork for the current TaskKind and hides the blockout body if it has one.
void ACityTaskActor::UpdateSprite()
{
    UPaperSprite* Wanted =
        StationSprites.IsValidIndex(static_cast<int32>(TaskKind))
        ? StationSprites[static_cast<int32>(TaskKind)].Get()
        : nullptr;

    if (Wanted)
    {
        ZeroDaySprites::ConfigureFlat(
            TaskSprite,
            Wanted,
            ZeroDaySprites::UprightRotation(),
            8.f
        );

        TaskMesh->SetVisibility(false);
    }
}

// Attending costs one of this station's resource. If the player can't pay, nothing happens.
void ACityTaskActor::Attend()
{
    if (!bIsActiveThisLevel)
    {
        return;
    }

    if (AZeroDayGameMode* GameMode =
        Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        const EResourceType Cost = ResourceForTask(TaskKind);

        if (!GameMode->SpendResource(Cost, 1))
        {
            GameMode->PostAlert(
                FString::Printf(
                    TEXT("The %s station needs %s. Collect some first."),
                    TaskKindToString(TaskKind),
                    ResourceTypeToString(Cost)
                ),
                FColor::Orange
            );

            return;
        }

        GameMode->PostEffect(
            GetActorLocation(),
            FLinearColor::Green,
            150.f,
            0.6f
        );
    }

    TaskComp->Attend();

    // Play station attended audio after the station has been successfully attended.
    if (AZeroDayCharacter* Character =
        Cast<AZeroDayCharacter>(
            UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if (UZeroDayAudioComponent* AudioComponent =
            Character->GetAudioComponent())
        {
            AudioComponent->PlayStationAttended(
                GetActorLocation()
            );
        }
    }
}

// Current condition (0-100), for the HUD.
float ACityTaskActor::GetCondition() const
{
    return TaskComp->GetCondition();
}

// Inactive stations are hidden and stop decaying.
void ACityTaskActor::SetActiveThisLevel(bool bActive)
{
    bIsActiveThisLevel = bActive;
    SetActorHiddenInGame(!bActive);
    TaskComp->SetComponentTickEnabled(bActive);
}

// Puts the station back to a clean state for a fresh level.
void ACityTaskActor::ResetForLevel(float DecayScale)
{
    GraceTimer->CancelCooldown();
    TaskComp->ResetTask(DecayScale);
}

// Condition hit the threshold: the player now has GracePeriodSeconds to respond.
void ACityTaskActor::HandleBecameCritical()
{
    GraceTimer->StartCooldown(
        GracePeriodSeconds,
        false
    );
}

// Player responded in time, so the grace period is cancelled.
void ACityTaskActor::HandleAttended()
{
    GraceTimer->CancelCooldown();
}

// Ran out of time while still critical: the task has failed and a survivor is lost.
void ACityTaskActor::HandleGraceExpired()
{
    if (TaskComp->IsCritical())
    {
        OnTaskFailed.Broadcast(this);

        // Play the station failed sound when the player runs out of time.
        if (AZeroDayCharacter* Character =
            Cast<AZeroDayCharacter>(
                UGameplayStatics::GetPlayerPawn(this, 0)))
        {
            if (UZeroDayAudioComponent* AudioComponent =
                Character->GetAudioComponent())
            {
                AudioComponent->PlayStationFailed(
                    GetActorLocation()
                );
            }
        }

        TaskComp->RestoreCondition(50.f);
    }
}
