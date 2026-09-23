#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ThreatPawn.generated.h"

class AWallActor;
class USphereComponent;
class UStaticMeshComponent;

// A single attacker (zombie stand-in for a threat/exploit attempt). Walks
// straight at its TargetWall and deals contact damage on arrival, or can be
// killed first by an AZeroDayProjectile (the SHOOT/contain remediation path).
UCLASS()
class ZERODAYLASTLINE_API AThreatPawn : public APawn
{
	GENERATED_BODY()

public:
	AThreatPawn();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float MoveSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float Health = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat")
	float ContactDamage = 20.f;

	UPROPERTY(BlueprintReadWrite, Category = "Threat")
	TObjectPtr<AWallActor> TargetWall;

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void ApplyHitDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void OnReachedTargetWall();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat")
	TObjectPtr<USphereComponent> HitSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
};
