#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Actor.h" // Base class for anything placed in the level.
#include "ZeroDayTypes.h" // ECityTaskKind.
#include "CityTaskActor.generated.h" // Unreal's generated reflection code (must be last include).

class UCityTaskComponent; // Forward declaration: the decaying meter.
class UCooldownComponent; // Forward declaration: the grace timer.
class UStaticMeshComponent; // Forward declaration: the solid station body.
class USceneComponent; // Forward declaration: the invisible root.
class UPaperSpriteComponent; // Forward declaration: the station artwork.
class UPaperSprite; // Forward declaration: a piece of station artwork.
class ACityTaskActor; // Forward declaration so the delegate below can name this class.

// Fired when a station runs out its grace period while still critical.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityTaskFailed, ACityTaskActor*, FailedTask); // Carries the station that failed.

// Power / food / medic tasks inside the walls. This is the other half of Validate &
// Prioritise: the player has to choose between running to a wall or attending a task
// before its grace period (GraceTimer) runs out.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API ACityTaskActor : public AActor // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	ACityTaskActor(); // Builds the mesh and components.

	// Which task this station represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask") // Editable in the editor and Blueprints.
	ECityTaskKind TaskKind = ECityTaskKind::Power; // Defaults to the generator.

	// Seconds the player has to attend the task once it turns critical.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask") // Editable in the editor and Blueprints.
	float GracePeriodSeconds = 8.f; // Eight seconds to respond.

	// How many times bigger than its original size the station is drawn and collides.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask") // Editable in the editor and Blueprints.
	float StationScale = 3.f; // Stations are three times their original size.

	// Set by the game mode: stations only run on level 3.
	UPROPERTY(BlueprintReadOnly, Category = "CityTask") // Readable from Blueprints.
	bool bIsActiveThisLevel = true; // True while the station is in play.

	// Fired if the grace period runs out while the task is still critical.
	UPROPERTY(BlueprintAssignable, Category = "CityTask") // Other classes can subscribe to it.
	FOnCityTaskFailed OnTaskFailed; // The game mode listens to this to remove a survivor.

	// Spends this station's resource (food, power cells or medical kits) and resets the task, if the player can afford it.
	UFUNCTION(BlueprintCallable, Category = "CityTask") // Callable from Blueprints.
	void Attend(); // The E key's station action.

	// Current condition (0-100), for the HUD.
	UFUNCTION(BlueprintPure, Category = "CityTask") // A read-only getter.
	float GetCondition() const; // Reads the decay meter.

	// How far from the station's centre the player may stand and still use it (its half-size, scaled).
	UFUNCTION(BlueprintPure, Category = "CityTask") // A read-only getter.
	float GetInteractRadius() const; // Grows with StationScale so a bigger station is easier to reach.

	// Chooses which station this is and shows the matching artwork.
	UFUNCTION(BlueprintCallable, Category = "CityTask") // Callable from Blueprints.
	void SetTaskKind(ECityTaskKind NewKind); // Called by the game mode when it builds the city.

	// Shows/hides the station and pauses its decay for the current level.
	UFUNCTION(BlueprintCallable, Category = "CityTask") // Callable from Blueprints.
	void SetActiveThisLevel(bool bActive); // Called at the start of every level.

	// Puts the station back to a full meter with no countdown, using the level's decay speed.
	void ResetForLevel(float DecayScale); // Called at the start of every level.

protected:
	virtual void BeginPlay() override; // Wires the meter to the grace timer.

	// Invisible root so the body's scale does not distort the artwork.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask") // Visible in the editor.
	TObjectPtr<USceneComponent> TaskRoot; // Everything attaches to this.

	// Solid station body the player walks around. Hidden when artwork is available.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask") // Visible in the editor.
	TObjectPtr<UStaticMeshComponent> TaskMesh; // The blockout cube (its collision always stays).

	// The station's artwork, which depends on TaskKind.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask") // Visible in the editor.
	TObjectPtr<UPaperSpriteComponent> TaskSprite; // Draws the picture.

	// One picture per station kind, indexed by ECityTaskKind.
	UPROPERTY() // Reflected so the sprites are not garbage collected.
	TArray<TObjectPtr<UPaperSprite>> StationSprites; // Power, Food, Medic.

	// The decaying condition meter.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask") // Visible in the editor.
	TObjectPtr<UCityTaskComponent> TaskComp; // Drains over time.

	// Countdown started when the task turns critical.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask") // Visible in the editor.
	TObjectPtr<UCooldownComponent> GraceTimer; // Expiry fails the station.

private:
	// Shows the artwork for the current TaskKind.
	void UpdateSprite(); // Also hides the blockout body.

	// Runs when the meter first drops to the critical threshold.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleBecameCritical(); // Starts the grace countdown.

	// Runs when the player attends the station.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleAttended(); // Cancels the grace countdown.

	// Runs when the grace period ends.
	UFUNCTION() // Must be reflected to be bound to a delegate.
	void HandleGraceExpired(); // Fails the station if it is still critical.

};
