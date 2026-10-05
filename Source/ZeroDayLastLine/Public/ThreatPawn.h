#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Pawn.h" // Base class: a simple thing that can move around.
#include "ZeroDayTypes.h" // EThreatSeverity.
#include "ThreatPawn.generated.h" // Unreal's generated reflection code (must be last include).

class AWallActor; // Forward declaration: the wall this threat is heading for.
class USphereComponent; // Forward declaration: the collision body.
class UStaticMeshComponent; // Forward declaration: the visible stand-in mesh.
class UPaperSpriteComponent; // Forward declaration: the sprite artwork.

// A single attacker (zombie stand-in for a threat/exploit attempt). Walks straight at its
// TargetWall and deals contact damage on arrival, or can be killed first by an
// AZeroDayProjectile (the SHOOT/contain remediation path).
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AThreatPawn : public APawn // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AThreatPawn(); // Builds the collision body and mesh.

	// Walking speed in units per second (before the level's speed scale).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat") // Editable in the editor and Blueprints.
	float MoveSpeed = 150.f; // Base speed.

	// Base health before the severity and level multipliers are applied.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat") // Editable in the editor and Blueprints.
	float Health = 30.f; // Base health.

	// Base damage dealt to the wall on arrival, before the severity multiplier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat") // Editable in the editor and Blueprints.
	float ContactDamage = 20.f; // Base damage.

	// How many times bigger than its original size the threat is drawn and collides.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threat") // Editable in the editor and Blueprints.
	float VisualScale = 4.f; // Threats are four times their original size.

	// Health when the threat spawned (after scaling), so the HUD can draw a health bar.
	UPROPERTY(BlueprintReadOnly, Category = "Threat") // Readable from Blueprints.
	float MaxHealth = 30.f; // Set when the threat is initialised.

	// The wall this threat is heading for.
	UPROPERTY(BlueprintReadWrite, Category = "Threat") // Readable and writable from Blueprints.
	TObjectPtr<AWallActor> TargetWall; // Set by the spawner.

	// How dangerous this threat is. Only revealed to the player by Validate.
	UPROPERTY(BlueprintReadOnly, Category = "Threat") // Readable from Blueprints.
	EThreatSeverity Severity = EThreatSeverity::Medium; // Low, Medium or Critical.

	// A false positive: looks like an exposure but is not exploitable, so it does no damage.
	UPROPERTY(BlueprintReadOnly, Category = "Threat") // Readable from Blueprints.
	bool bIsDecoy = false; // True for harmless threats.

	// Called by the spawner right after spawning. Assigns the target, severity and decoy
	// status, and scales speed and health for the current level.
	void InitialiseThreat(AWallActor* InTargetWall, EThreatSeverity InSeverity, bool bInIsDecoy, float SpeedScale = 1.f, float HealthScale = 1.f); // Sets the threat's identity and stats.

	// Insta Scan flags a threat as detected; Validate then reveals what it really is.
	void MarkDetected() { bDetected = true; } // Set by a scan.
	void MarkValidated() { bValidated = true; } // Set by validation.
	bool IsDetected() const { return bDetected; } // Has a scan found this threat?
	bool IsValidated() const { return bValidated; } // Has validation revealed its identity?

	// Health left as a 0-1 fraction, for health bars.
	float GetHealthFraction() const { return MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f; } // Safe against a zero maximum.

	// Radius of the threat's collision body, in world units (grows with VisualScale), for the HUD.
	float GetBodyRadius() const; // Used to size markers and health bars.

	// Called by projectiles. The threat is removed once its health runs out.
	UFUNCTION(BlueprintCallable, Category = "Threat") // Callable from Blueprints.
	void ApplyHitDamage(float Amount); // Lose health; die at zero.

	// Damages the wall (0 for false positives) and removes the threat.
	UFUNCTION(BlueprintCallable, Category = "Threat") // Callable from Blueprints.
	void OnReachedTargetWall(); // Called by the wall when the threat touches it.

protected:
	virtual void BeginPlay() override; // Runs when the threat enters the world.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override; // Runs when the threat leaves the world.
	virtual void Tick(float DeltaTime) override; // Runs every frame to walk towards the wall.

	bool bDetected = false; // True once an Insta Scan has flagged this threat.
	bool bValidated = false; // True once Validate has revealed this threat.

	// Collision body: overlaps everything, blocks on walls.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat") // Visible in the editor.
	TObjectPtr<USphereComponent> HitSphere; // Where shots and walls detect the threat.

	// Visual stand-in only, no collision. Hidden when the sprite artwork is available.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat") // Visible in the editor.
	TObjectPtr<UStaticMeshComponent> BodyMesh; // The blockout cylinder.

	// The threat's artwork. It turns with the threat so it faces the way it walks.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Threat") // Visible in the editor.
	TObjectPtr<UPaperSpriteComponent> BodySprite; // Draws the picture.
};
