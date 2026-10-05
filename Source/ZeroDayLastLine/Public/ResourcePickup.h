#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayTypes.h" // EResourceType.
#include "ResourcePickup.generated.h" // Unreal's generated reflection code (must be last include).

class USphereComponent; // Forward declaration: the physical body.
class UPaperSpriteComponent; // Forward declaration: the picture.

// A collectable resource. It is a real physics object: it is flung out when a threat dies or a
// supply drop arrives, bounces off walls and other pickups, then slides to a stop. Each type has
// its own physics material, so a springy Medical case bounces much more than a Food crate.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AResourcePickup : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AResourcePickup(); // Builds the physics body and sprite.

	// What this pickup gives the player.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup") // Editable in the editor and Blueprints.
	EResourceType ResourceType = EResourceType::Intel; // What it gives.

	// How many of the resource it gives.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup") // Editable in the editor and Blueprints.
	int32 Amount = 1; // How many it gives.

	// Chooses the pickup's type (picture and physics material) and flings it with a starting velocity.
	void Initialise(EResourceType InType, const FVector& LaunchVelocity); // Sets its type and launches it.

	// Spawns a pickup at a location and launches it in a random direction at roughly the given speed.
	static AResourcePickup* SpawnScattered(UWorld* World, EResourceType Type, const FVector& Location, float LaunchSpeed); // Spawns and throws one.

	// The physics body (its material, mass and velocity are what the physics tests inspect).
	USphereComponent* GetBody() const { return Body; } // Used by the tests.

protected:
	virtual void BeginPlay() override; // Hooks up the overlap event.

	// The physics body. It simulates, bounces and slides.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup") // Visible in the editor.
	TObjectPtr<USphereComponent> Body; // The physics body.

	// The picture. Kept upright on screen even while the body tumbles.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup") // Visible in the editor.
	TObjectPtr<UPaperSpriteComponent> Icon; // The picture.

	// One picture per resource type, indexed by EResourceType.
	UPROPERTY() // Reflected so the sprites are not garbage collected.
	TArray<TObjectPtr<class UPaperSprite>> Sprites; // One sprite per resource type.

private:
	// Runs when something overlaps the pickup: the player collects it.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult); // The player touched it.
};
