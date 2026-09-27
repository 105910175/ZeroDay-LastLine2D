#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZeroDayProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

// Player-fired shot for the SHOOT/contain remediation path.
UCLASS()
class ZERODAYLASTLINE_API AZeroDayProjectile : public AActor
{
	GENERATED_BODY()

public:
	AZeroDayProjectile();

	// Damage dealt to a threat on hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 15.f;

	// Travel speed in units per second.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed = 3000.f;

protected:
	virtual void BeginPlay() override;

	// Projectile channel (GameTraceChannel3): overlaps threats, blocks on static level geometry.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	// Visual only, no collision.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// Flies in a straight line with no gravity.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComp;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
