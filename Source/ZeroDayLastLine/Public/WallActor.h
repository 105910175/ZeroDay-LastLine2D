#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZeroDayTypes.h"
#include "WallActor.generated.h"

class UBreachableComponent;
class UCooldownComponent;
class UStaticMeshComponent;
class UBoxComponent;
class AWallActor;
class AThreatPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallLost, AWallActor*, LostWall);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallStatusUpdated, AWallActor*, Wall, EWallStatus, NewStatus);

// One of the four city walls. Detect = Breachable's Threatened/Breached status.
// Remediate = AttemptFix() (patch) or simply surviving the RemediationTimer window
// by killing off the threats attacking it (contain).
UCLASS()
class ZERODAYLASTLINE_API AWallActor : public AActor
{
	GENERATED_BODY()

public:
	AWallActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EWallSide WallSide = EWallSide::North;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	float RemediationWindowSeconds = 12.f;

	UPROPERTY(BlueprintReadOnly, Category = "Wall")
	bool bIsActiveThisLevel = true;

	UPROPERTY(BlueprintAssignable, Category = "Wall")
	FOnWallLost OnWallLost;

	UPROPERTY(BlueprintAssignable, Category = "Wall")
	FOnWallStatusUpdated OnStatusUpdated;

	// Threats currently walking towards this wall. Threats register themselves when
	// initialised and unregister when destroyed.
	void RegisterInboundThreat(AThreatPawn* Threat);
	void UnregisterInboundThreat(AThreatPawn* Threat);

	int32 GetInboundThreatCount() const;

	// Number of inbound threats an Insta Scan has flagged / that Validate hasn't checked yet.
	int32 GetDetectedThreatCount() const;
	int32 GetUnvalidatedDetectedCount() const;
	bool IsExposureDetected() const { return GetDetectedThreatCount() > 0; }

	// Insta Scan: flags every inbound threat as detected. Returns how many there are.
	int32 MarkInboundDetected();

	// Validate: reveals whether each detected threat is real or a false positive.
	void ValidateInbound(int32& OutReal, int32& OutFalsePositives, EThreatSeverity& OutHighestSeverity);

	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetHealthPercent() const;

	// Seconds left to remediate a breach (0 when not breached).
	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetRemediationTimeRemaining() const;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeThreatDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void AttemptFix();

	UFUNCTION(BlueprintPure, Category = "Wall")
	EWallStatus GetStatus() const;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void SetActiveThisLevel(bool bActive);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> ThreatContactZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBreachableComponent> Breachable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UCooldownComponent> RemediationTimer;

private:
	TArray<TWeakObjectPtr<AThreatPawn>> InboundThreats;

	UFUNCTION()
	void HandleStatusChanged(EWallStatus NewStatus);

	UFUNCTION()
	void HandleRemediationExpired();

	UFUNCTION()
	void HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
