#include "ResourcePickup.h" // This class's own declaration.
#include "ZeroDayCharacter.h" // Only the player can collect pickups.
#include "Audio/ZeroDayAudioComponent.h"
#include "ZeroDayGameMode.h" // Where collected resources go.
#include "ZeroDayPhysics.h" // Physics materials.
#include "ZeroDaySprites.h" // Helper that lays sprites flat.
#include "PaperSprite.h" // The picture asset type.
#include "PaperSpriteComponent.h" // Draws the picture.
#include "Components/SphereComponent.h" // The physics body.
#include "Kismet/GameplayStatics.h" // Lets us fetch the current game mode.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load content assets.
#include "PhysicalMaterials/PhysicalMaterial.h" // So the material can be applied to the body.

namespace // Only this file uses these.
{
	constexpr float BodyRadius = 30.f; // Physical size of a pickup.
	constexpr float PickupLifetimeSeconds = 35.f; // Uncollected pickups disappear after this long.
	const TCHAR* SpritePaths[ZeroDayResourceTypeCount] = { // One picture per resource type, in enum order.
		TEXT("/Game/Sprites/S_pickup_intel.S_pickup_intel"),
		TEXT("/Game/Sprites/S_pickup_patch.S_pickup_patch"),
		TEXT("/Game/Sprites/S_pickup_food.S_pickup_food"),
		TEXT("/Game/Sprites/S_pickup_power.S_pickup_power"),
		TEXT("/Game/Sprites/S_pickup_medical.S_pickup_medical"),
	};
}

// Builds the physics body and the picture.
AResourcePickup::AResourcePickup()
{
	PrimaryActorTick.bCanEverTick = false; // Physics moves it; no per-frame logic needed.
	InitialLifeSpan = PickupLifetimeSeconds; // Clean up uncollected pickups automatically.

	Body = CreateDefaultSubobject<USphereComponent>(TEXT("Body")); // The physical body.
	RootComponent = Body; // It is the root, so the whole actor moves with the physics simulation.
	Body->SetSphereRadius(BodyRadius); // Give it a size.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // It collides and is found by overlaps.
	Body->SetCollisionObjectType(ECC_GameTraceChannel4); // Pickup channel (GameTraceChannel4).
	Body->SetCollisionResponseToAllChannels(ECR_Ignore); // Ignore everything by default...
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // ...bounce off walls...
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Block); // ...and off other pickups...
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // ...and get knocked by projectiles...
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // ...but let the player walk over them to collect.
	Body->SetSimulatePhysics(true); // Let the physics engine move it.
	Body->SetEnableGravity(false); // This is a top-down game: nothing falls.
	Body->SetConstraintMode(EDOFMode::XYPlane); // Keep it sliding on the ground plane, never lifting off or tipping over.
	Body->SetLinearDamping(1.4f); // Slows it down like ground friction, so it comes to rest.
	Body->SetAngularDamping(3.f); // Stops it spinning forever.

	Icon = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Icon")); // The picture.
	Icon->SetupAttachment(Body); // It rides along with the body.
	Icon->SetUsingAbsoluteRotation(true); // But stays upright on screen even while the body tumbles.

	Sprites.SetNum(ZeroDayResourceTypeCount); // Room for one picture per type.
	for (int32 Index = 0; Index < ZeroDayResourceTypeCount; ++Index) // Load each picture.
	{
		ConstructorHelpers::FObjectFinder<UPaperSprite> Finder(SpritePaths[Index]); // Look it up (a new finder each time; the paths differ).
		if (Finder.Succeeded()) // Found it.
		{
			Sprites[Index] = Finder.Object; // Remember it.
		}
	}
}

// Listen for the player touching the pickup.
void AResourcePickup::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.
	Body->OnComponentBeginOverlap.AddDynamic(this, &AResourcePickup::HandleOverlap); // Call HandleOverlap when something touches it.
	Initialise(ResourceType, FVector::ZeroVector); // Apply the picture and material for the default type (a spawner re-applies for its own type).
}

// Chooses the picture and physics material for a type, then launches the pickup.
void AResourcePickup::Initialise(EResourceType InType, const FVector& LaunchVelocity)
{
	ResourceType = InType; // Remember what this pickup is.
	const int32 Index = static_cast<int32>(InType); // Index into the picture list.

	if (Sprites.IsValidIndex(Index) && Sprites[Index]) // Do we have a picture for this type?
	{
		ZeroDaySprites::ConfigureFlat(Icon, Sprites[Index], ZeroDaySprites::UprightRotation(), 8.f); // Show it flat and upright.
	}

	Body->SetPhysMaterialOverride(ZeroDayPhysics::Get(ZeroDayPhysics::ForResource(InType))); // Give the body this type's bounce, grip and weight.
	Body->SetPhysicsLinearVelocity(LaunchVelocity); // Fling it (zero just leaves it where it is).
}

// Spawns a pickup and throws it in a random direction.
AResourcePickup* AResourcePickup::SpawnScattered(UWorld* World, EResourceType Type, const FVector& Location, float LaunchSpeed)
{
	if (!World) // Cannot spawn without a world.
	{
		return nullptr; // Report failure.
	}

	FActorSpawnParameters Params; // Settings for the spawn call.
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // Always appear, even if overlapping something.
	AResourcePickup* Pickup = World->SpawnActor<AResourcePickup>(AResourcePickup::StaticClass(), Location, FRotator::ZeroRotator, Params); // Create it.
	if (Pickup) // Spawn worked.
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI); // Random direction around the circle.
		const float Speed = LaunchSpeed * FMath::FRandRange(0.7f, 1.2f); // Vary the speed a little.
		Pickup->Initialise(Type, FVector(FMath::Cos(Angle) * Speed, FMath::Sin(Angle) * Speed, 0.f)); // Set its type and throw it.
	}
	return Pickup; // Hand it back.
}

// The player walking over a pickup collects it.
void AResourcePickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AZeroDayCharacter>(OtherActor)) // Only the player can pick things up.
	{
		return; // Ignore anything else.
	}

	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
	{
		if (GameMode->IsGameOver()) // Nothing to collect once the level is over.
		{
			return; // Leave it.
		}
        GameMode->AddResource(ResourceType, Amount);

        if (AZeroDayCharacter* Character = Cast<AZeroDayCharacter>(OtherActor))
        {
            if (UZeroDayAudioComponent* AudioComponent =
                Character->GetAudioComponent())
            {
                AudioComponent->PlayPickup(GetActorLocation());
            }
        }

        GameMode->RecordSupplyCollected();
        GameMode->PostAlert(
            FString::Printf(
                TEXT("+%d %s"),
                Amount,
                ResourceTypeToString(ResourceType)
            ),
            FColor(150, 240, 160)
        );
        GameMode->PostEffect(
            GetActorLocation(),
            FLinearColor(0.5f, 1.f, 0.55f),
            90.f,
            0.5f
        );
    }

    Destroy(); // The pickup is used up.
}
