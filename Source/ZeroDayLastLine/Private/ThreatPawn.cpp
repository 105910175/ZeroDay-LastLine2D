#include "ThreatPawn.h"
#include "WallActor.h"
#include "ZeroDayGameMode.h"
#include "ZeroDayPhysics.h"
#include "ZeroDaySprites.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "ZeroDayCharacter.h"
#include "Audio/ZeroDayAudioComponent.h"

// Threat channel (GameTraceChannel2).
AThreatPawn::AThreatPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
    RootComponent = HitSphere;
    HitSphere->SetSphereRadius(50.f);
    HitSphere->SetCollisionObjectType(ECC_GameTraceChannel2);
    HitSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    HitSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
    );

    if (CylinderMeshAsset.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMeshAsset.Object);
    }

    BodyMesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.0f));
    BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -50.f));

    BodySprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("BodySprite"));
    BodySprite->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UPaperSprite> SpriteFinder(
        TEXT("/Game/Sprites/S_threat.S_threat")
    );

    if (SpriteFinder.Succeeded())
    {
        ZeroDaySprites::ConfigureFlat(
            BodySprite,
            SpriteFinder.Object,
            ZeroDaySprites::HeadingRotation(),
            4.f
        );

        BodyMesh->SetVisibility(false);
    }
}

// Nothing extra to do when the threat appears.
void AThreatPawn::BeginPlay()
{
    Super::BeginPlay();

    HitSphere->SetPhysMaterialOverride(
        ZeroDayPhysics::Get(EZeroDayMaterial::Threat)
    );
}

// Tell the wall this threat is gone.
void AThreatPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (TargetWall)
    {
        TargetWall->UnregisterInboundThreat(this);
    }

    Super::EndPlay(EndPlayReason);
}

// Applies the target, severity and decoy status.
void AThreatPawn::InitialiseThreat(
    AWallActor* InTargetWall,
    EThreatSeverity InSeverity,
    bool bInIsDecoy,
    float SpeedScale,
    float HealthScale)
{
    TargetWall = InTargetWall;
    Severity = InSeverity;
    bIsDecoy = bInIsDecoy;

    float DamageScale = 1.f;
    float SeverityHealthScale = 1.f;

    switch (Severity)
    {
    case EThreatSeverity::Low:
        DamageScale = 0.5f;
        SeverityHealthScale = 0.7f;
        break;

    case EThreatSeverity::Critical:
        DamageScale = 2.f;
        SeverityHealthScale = 2.f;
        break;

    default:
        break;
    }

    ContactDamage = bIsDecoy ? 0.f : ContactDamage * DamageScale;
    Health *= SeverityHealthScale * HealthScale;
    MoveSpeed *= SpeedScale;
    MaxHealth = Health;

    if (TargetWall)
    {
        TargetWall->RegisterInboundThreat(this);
    }
}

// Walks in a straight line at the target wall.
void AThreatPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TargetWall)
    {
        return;
    }

    const FVector ToTarget =
        TargetWall->GetActorLocation() - GetActorLocation();

    const FVector FlatDirection = FVector(
        ToTarget.X,
        ToTarget.Y,
        0.f
    ).GetSafeNormal();

    if (!FlatDirection.IsNearlyZero())
    {
        AddActorWorldOffset(
            FlatDirection * MoveSpeed * DeltaTime,
            true
        );

        SetActorRotation(FlatDirection.Rotation());
    }
}

// Called by projectiles.
void AThreatPawn::ApplyHitDamage(float Amount)
{
    Health -= Amount;

    if (Health <= 0.f)
    {
        if (AZeroDayGameMode* GameMode =
            Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->OnThreatKilled(this);
        }

        // Play the threat destroyed sound.
        if (AZeroDayCharacter* Character =
            Cast<AZeroDayCharacter>(GetInstigator()))
        {
            if (UZeroDayAudioComponent* AudioComponent =
                Character->GetAudioComponent())
            {
                AudioComponent->PlayThreatDestroyed(
                    GetActorLocation()
                );
            }
        }

        Destroy();
    }
}

// Damages the wall and removes the threat.
void AThreatPawn::OnReachedTargetWall()
{
    if (TargetWall)
    {
        TargetWall->TakeThreatDamage(ContactDamage);
    }

    Destroy();
}
