#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ZeroDayTypes.h"
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

	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	EThreatSeverity Severity = EThreatSeverity::Medium;

	// A false positive: looks like an exposure but is not exploitable, so it does no damage.
	UPROPERTY(BlueprintReadOnly, Category = "Threat")
	bool bIsDecoy = false;

	// Called by the spawner right after spawning. Assigns the target, rolls severity
	// and decoy status, and scales stats to match.
	void InitialiseThreat(AWallActor* InTargetWall, EThreatSeverity InSeverity, bool bInIsDecoy);

	// Insta Scan flags a threat as detected; Validate then reveals what it really is.
	void MarkDetected() { bDetected = true; }
	void MarkValidated() { bValidated = true; }
	bool IsDetected() const { return bDetected; }
	bool IsValidated() const { return bValidated; }

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void ApplyHitDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Threat")
	void OnReachedTargetWall();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	bool bDetected = false;
	bool bValidated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat")
	TObjectPtr<USphereComponent> HitSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
};
