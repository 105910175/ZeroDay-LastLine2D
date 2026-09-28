#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayTypes.h" // EWallSide, EWallStatus, EThreatSeverity.
#include "WallActor.generated.h" // Unreal's generated reflection code (must be last include).

class UBreachableComponent; // Forward declaration: tracks health and status.
class UCooldownComponent; // Forward declaration: the breach countdown timer.
class USceneComponent; // Forward declaration: the invisible root everything hangs off.
class UStaticMeshComponent; // Forward declaration: the solid wall body.
class UBoxComponent; // Forward declaration: trigger volume for arriving threats.
class UPaperSpriteComponent; // Forward declaration: draws one tile of the wall's artwork.
class UPaperSprite; // Forward declaration: a piece of wall artwork.
class AWallActor; // Forward declaration so the delegates below can name this class.
class AThreatPawn; // Forward declaration: the attackers heading for this wall.

// Fired when a wall stays breached until its countdown ends (a fail condition).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallLost, AWallActor*, LostWall);
// Fired whenever a wall's status changes (Safe / Threatened / Breached).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallStatusUpdated, AWallActor*, Wall, EWallStatus, NewStatus);

// One of the four city walls. Detect = the Threatened/Breached status and scan markers.
// Remediate = AttemptFix() (patch) or clearing the threats attacking it (contain).
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AWallActor : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AWallActor(); // Builds the mesh, trigger zone and components.

	// Which side of the city this wall guards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EWallSide WallSide = EWallSide::North;

	// How long the wall is along its long axis, in world units.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	float WallLength = 300.f;

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

	// Threats register themselves here when initialised and unregister when destroyed.
	void RegisterInboundThreat(AThreatPawn* Threat);
	void UnregisterInboundThreat(AThreatPawn* Threat);

	// How many live threats are heading for this wall.
	int32 GetInboundThreatCount() const;

	// Number of inbound threats an Insta Scan has flagged / that Validate hasn't checked yet.
	int32 GetDetectedThreatCount() const;
	int32 GetUnvalidatedDetectedCount() const;
	bool IsExposureDetected() const { return GetDetectedThreatCount() > 0; } // True if any inbound threat is flagged.

	// Insta Scan: flags every inbound threat as detected. Returns how many there are.
	int32 MarkInboundDetected();

	// Validate: reveals whether each detected threat is real or a false positive.
	void ValidateInbound(int32& OutReal, int32& OutFalsePositives, EThreatSeverity& OutHighestSeverity);

	// Wall health as a 0-1 fraction, for the HUD.
	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetHealthPercent() const;

	// Seconds left to remediate a breach (0 when not breached).
	UFUNCTION(BlueprintPure, Category = "Wall")
	float GetRemediationTimeRemaining() const;

	// Called by a threat when it reaches the wall.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeThreatDamage(float Amount);

	// The player's patch action: costs a Patch kit and fully repairs the wall if it is breached. Returns true if it fixed something.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	bool AttemptFix();

	// Current Safe / Threatened / Breached status.
	UFUNCTION(BlueprintPure, Category = "Wall")
	EWallStatus GetStatus() const;

	// Shows/hides the wall and enables/disables its collision for the current level.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void SetActiveThisLevel(bool bActive);

	// Resizes the wall (its body, trigger zone and artwork) to a new length.
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void SetWallLength(float NewLength);

	// Applies a level's rules to this wall (currently just the breach countdown length).
	void ApplyLevelSettings(float NewRemediationWindowSeconds);

	// Puts the wall back to full health with no breach countdown, ready for a fresh level.
	void ResetForLevel();

protected:
	virtual void BeginPlay() override; // Binds the component events once everything exists.

	// Invisible root so the mesh's scale does not affect the artwork.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<USceneComponent> WallRoot;

	// Solid wall body (blocks the player, threats and pickups). Hidden when artwork is available.
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

	// The wall's artwork for each condition.
	UPROPERTY()
	TObjectPtr<UPaperSprite> IntactSprite;

	UPROPERTY()
	TObjectPtr<UPaperSprite> DamagedSprite;

	UPROPERTY()
	TObjectPtr<UPaperSprite> BreachedSprite;

	// The artwork is repeated along the wall in tiles, so any length looks right.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPaperSpriteComponent>> SpriteTiles;

private:
	// Threats currently heading for this wall. Weak pointers so destroyed threats drop out safely.
	TArray<TWeakObjectPtr<AThreatPawn>> InboundThreats;

	// True when the artwork assets were found, so the blockout mesh can stay hidden.
	bool bHasSprites = false;

	// Rebuilds the row of sprite tiles for the current length.
	void RebuildVisuals();

	// Switches every tile to the artwork matching the wall's current condition.
	void UpdateVisuals();

	// Reacts to the Breachable component changing status.
	UFUNCTION()
	void HandleStatusChanged(EWallStatus NewStatus);

	// Reacts to the breach countdown running out.
	UFUNCTION()
	void HandleRemediationExpired();

	// Reacts to something entering the threat trigger zone.
	UFUNCTION()
	void HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
