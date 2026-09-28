#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayProjectile.generated.h" // Unreal's generated reflection code (must be last include).

class USphereComponent; // Forward declaration: the collision body.
class UProjectileMovementComponent; // Forward declaration: flies the shot in a straight line.
class UStaticMeshComponent; // Forward declaration: the blockout visual.
class UPaperSpriteComponent; // Forward declaration: the bolt artwork.

// Player-fired shot for the SHOOT/contain remediation path.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AZeroDayProjectile : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AZeroDayProjectile(); // Builds the body, visuals and movement.

	// Damage dealt to a threat on hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 15.f;

	// Travel speed in units per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed = 3000.f;

	// How fast (units per second) a shot knocks a resource pickup when it hits one.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float PickupKnockSpeed = 350.f;

protected:
	virtual void BeginPlay() override; // Hooks up the collision events.

	// Projectile channel (GameTraceChannel3): overlaps threats, blocks on static level geometry and pickups.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	// Visual only, no collision. Hidden when the bolt artwork is available.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// The bolt artwork, pointing the way the shot travels.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UPaperSpriteComponent> BoltSprite;

	// Flies in a straight line with no gravity.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComp;

private:
	// Runs when the shot overlaps something it can pass through (threats).
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Runs when the shot hits something solid it cannot pass through.
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
