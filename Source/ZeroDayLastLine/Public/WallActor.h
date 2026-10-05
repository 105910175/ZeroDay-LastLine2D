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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallLost, AWallActor*, LostWall); // Carries the wall that fell.
// Fired whenever a wall's status changes (Safe / Threatened / Breached).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallStatusUpdated, AWallActor*, Wall, EWallStatus, NewStatus); // Carries the wall and its new status.

// One of the four city walls. Detect = the Threatened/Breached status and scan markers.
// Remediate = AttemptFix() (patch) or clearing the threats attacking it (contain).
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AWallActor : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AWallActor(); // Builds the mesh, trigger zone and components.

	// Which side of the city this wall guards.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall") // Editable in the editor and Blueprints.
	EWallSide WallSide = EWallSide::North; // Defaults to the north wall.

	// How long the wall is along its long axis, in world units.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall") // Editable in the editor and Blueprints.
	float WallLength = 300.f; // The game mode overrides this when it builds the city.

	// How many times thicker (and taller) than its original size the wall is drawn and collides.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall") // Editable in the editor and Blueprints.
	float WallSizeScale = 2.f; // Walls are twice their original size.

	// Seconds the player has to fix or clear a breach before the wall is lost.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall") // Editable in the editor and Blueprints.
	float RemediationWindowSeconds = 12.f; // The level config overrides this.

	// Set by the game mode: walls not in play this level are hidden and ignored.
	UPROPERTY(BlueprintReadOnly, Category = "Wall") // Readable from Blueprints.
	bool bIsActiveThisLevel = true; // True while the wall is in play.

	// Fired if the remediation window expires while still breached (a fail condition).
	UPROPERTY(BlueprintAssignable, Category = "Wall") // Other classes can subscribe to it.
	FOnWallLost OnWallLost; // The game mode listens to this to end the level.

	// Fired whenever the wall's status changes; the game mode turns these into alerts.
	UPROPERTY(BlueprintAssignable, Category = "Wall") // Other classes can subscribe to it.
	FOnWallStatusUpdated OnStatusUpdated; // The game mode listens to this to post alerts.

	// Threats register themselves here when initialised and unregister when destroyed.
	void RegisterInboundThreat(AThreatPawn* Threat); // Adds a threat to this wall's list.
	void UnregisterInboundThreat(AThreatPawn* Threat); // Removes a threat from this wall's list.

	// How many live threats are heading for this wall.
	int32 GetInboundThreatCount() const; // Counts the ones still alive.

	// Number of inbound threats an Insta Scan has flagged / that Validate hasn't checked yet.
	int32 GetDetectedThreatCount() const; // Flagged by a scan.
	int32 GetUnvalidatedDetectedCount() const; // Flagged but not yet validated.
	bool IsExposureDetected() const { return GetDetectedThreatCount() > 0; } // True if any inbound threat is flagged.

	// Insta Scan: flags every inbound threat as detected. Returns how many there are.
	int32 MarkInboundDetected(); // Used by Insta Scan.

	// Validate: reveals whether each detected threat is real or a false positive.
	void ValidateInbound(int32& OutReal, int32& OutFalsePositives, EThreatSeverity& OutHighestSeverity); // Used by Validate.

	// Wall health as a 0-1 fraction, for the HUD.
	UFUNCTION(BlueprintPure, Category = "Wall") // A read-only getter.
	float GetHealthPercent() const; // Reads the health component.

	// Distance from a world point to the closest point along the wall's length, not just its
	// centre. Walls now span most of a city side, so "am I close enough to fix this wall" has
	// to check the nearest point on the wall, not the (possibly very far away) middle of it.
	UFUNCTION(BlueprintPure, Category = "Wall") // A read-only getter.
	float GetDistanceToPoint(const FVector& Point) const; // Used by the Interact key.

	// Seconds left to remediate a breach (0 when not breached).
	UFUNCTION(BlueprintPure, Category = "Wall") // A read-only getter.
	float GetRemediationTimeRemaining() const; // Reads the breach countdown.

	// Called by a threat when it reaches the wall.
	UFUNCTION(BlueprintCallable, Category = "Wall") // Callable from Blueprints.
	void TakeThreatDamage(float Amount); // Applies damage if the wall is in play.

	// The player's patch action: costs a Patch kit and fully repairs the wall if it is breached. Returns true if it fixed something.
	UFUNCTION(BlueprintCallable, Category = "Wall") // Callable from Blueprints.
	bool AttemptFix(); // The E key's wall action.

	// Current Safe / Threatened / Breached status.
	UFUNCTION(BlueprintPure, Category = "Wall") // A read-only getter.
	EWallStatus GetStatus() const; // Reads the health component.

	// Shows/hides the wall and enables/disables its collision for the current level.
	UFUNCTION(BlueprintCallable, Category = "Wall") // Callable from Blueprints.
	void SetActiveThisLevel(bool bActive); // Called at the start of every level.

	// Resizes the wall (its body, trigger zone and artwork) to a new length.
	UFUNCTION(BlueprintCallable, Category = "Wall") // Callable from Blueprints.
	void SetWallLength(float NewLength); // Called by the game mode when it builds the city.

	// Applies a level's rules to this wall (currently just the breach countdown length).
	void ApplyLevelSettings(float NewRemediationWindowSeconds); // Called at the start of every level.

	// Puts the wall back to full health with no breach countdown, ready for a fresh level.
	void ResetForLevel(); // Called at the start of every level.

protected:
	virtual void BeginPlay() override; // Binds the component events once everything exists.

	// Invisible root so the mesh's scale does not affect the artwork.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall") // Visible in the editor.
	TObjectPtr<USceneComponent> WallRoot; // Everything attaches to this.

	// Solid wall body (blocks the player, threats and pickups). Hidden when artwork is available.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall") // Visible in the editor.
	TObjectPtr<UStaticMeshComponent> WallMesh; // The blockout cube (its collision always stays).

	// Trigger volume that detects threats arriving at the wall.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall") // Visible in the editor.
	TObjectPtr<UBoxComponent> ThreatContactZone; // Fires when a threat touches it.

	// Health and Safe/Threatened/Breached status.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall") // Visible in the editor.
	TObjectPtr<UBreachableComponent> Breachable; // Tracks the wall's health.

	// Countdown started on breach; expiry means the wall is lost.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall") // Visible in the editor.
	TObjectPtr<UCooldownComponent> RemediationTimer; // The breach clock.

	// The wall's artwork for each condition.
	UPROPERTY() // Reflected so the sprites are not garbage collected.
	TObjectPtr<UPaperSprite> IntactSprite; // Healthy wall.

	UPROPERTY() // Reflected so the sprites are not garbage collected.
	TObjectPtr<UPaperSprite> DamagedSprite; // Cracked wall.

	UPROPERTY() // Reflected so the sprites are not garbage collected.
	TObjectPtr<UPaperSprite> BreachedSprite; // Broken wall.

	// The artwork is repeated along the wall in tiles, so any length looks right. These are
	// created once in the constructor (a fixed pool, enough for the longest wall we build) and
	// only shown/repositioned afterwards — dynamically creating and destroying components at
	// runtime turned out not to render reliably for this actor, so the wall is sized by toggling
	// visibility on a pre-built pool instead.
	static constexpr int32 MaxWallTiles = 48; // The most tiles any wall can use (a 3,400 unit wall needs about 34 tiles of 100 units).

	UPROPERTY(Transient) // Rebuilt every run, not saved.
	TArray<TObjectPtr<UPaperSpriteComponent>> SpriteTiles; // The pool of tile components.

private:
	// Threats currently heading for this wall. Weak pointers so destroyed threats drop out safely.
	TArray<TWeakObjectPtr<AThreatPawn>> InboundThreats; // Used by scan and validate.

	// True when the artwork assets were found, so the blockout mesh can stay hidden.
	bool bHasSprites = false; // False means the blockout cube is shown instead.

	// Rebuilds the row of sprite tiles for the current length.
	void RebuildVisuals(); // Shows and positions as many tiles as needed.

	// Switches every tile to the artwork matching the wall's current condition.
	void UpdateVisuals(); // Intact, cracked or broken.

	// Reacts to the Breachable component changing status.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleStatusChanged(EWallStatus NewStatus); // Updates artwork, announces and starts/stops the countdown.

	// Reacts to the breach countdown running out.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleRemediationExpired(); // Declares the wall lost.

	// Reacts to something entering the threat trigger zone.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult); // A threat has arrived.
};
