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

	// Which side of the city this wall guards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EWallSide WallSide = EWallSide::North;

	// Seconds the player has to fix or clear a breach before the wall is lost.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	float RemediationWindowSeconds = 12.f;

	// Set by the game mode: walls not in play this level are hidden and ignored.
	UPROPERTY(BlueprintReadOnly, Category = "Wall")
	bool bIsActiveThisLevel = true;

	// Fired if the remediation window expires while still breached (a fail condition).
	UPROPERTY(BlueprintAssignable, Category = "Wall")
	FOnWallLost OnWallLost;

	// Fired whenever the wall's status changes; the game mode turns these into alerts.
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

	// Called by a threat when it reaches the wall.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeThreatDamage(float Amount);

	// The player's patch action: fully repairs the wall if it is breached.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void AttemptFix();

	UFUNCTION(BlueprintPure, Category = "Wall")
	EWallStatus GetStatus() const;

	// Shows/hides the wall and enables/disables its collision for the current level.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void SetActiveThisLevel(bool bActive);

protected:
	virtual void BeginPlay() override;

	// Visible, solid wall (blocks the player and threats).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	// Trigger volume that detects threats arriving at the wall.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> ThreatContactZone;

	// Health and Safe/Threatened/Breached status.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBreachableComponent> Breachable;

	// Countdown started on breach; expiry means the wall is lost.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UCooldownComponent> RemediationTimer;

private:
	// Threats currently heading for this wall. Weak pointers so destroyed threats drop out safely.
	TArray<TWeakObjectPtr<AThreatPawn>> InboundThreats;

	UFUNCTION()
	void HandleStatusChanged(EWallStatus NewStatus);

	UFUNCTION()
	void HandleRemediationExpired();

	UFUNCTION()
	void HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
