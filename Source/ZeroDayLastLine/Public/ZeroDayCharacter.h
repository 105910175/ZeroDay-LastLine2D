// Fill out your copyright notice in the Description page of Project Settings.

#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/Character.h" // Base class for a walking player.
#include "ZeroDayCharacter.generated.h" // Unreal's generated reflection code (must be last include).

class USpringArmComponent; // Forward declaration: holds the camera.
class UCameraComponent; // Forward declaration: the top-down view.
class AZeroDayProjectile; // Forward declaration: what the player fires.
class UDetectionComponent; // Forward declaration: Insta Scan and Validate.
class UZeroDayAudioComponent; // Forward declaration: the sound effect player.

UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AZeroDayCharacter : public ACharacter // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	// Sets default values for this character's properties
	AZeroDayCharacter(); // Builds the camera, detection tools and audio player.

	// What the player fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat") // Editable in the editor and Blueprints.
	TSubclassOf<AZeroDayProjectile> ProjectileClass; // The class spawned by Fire().

	// How close the player must be to a wall or task to use it with E.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat") // Editable in the editor and Blueprints.
	float InteractRange = 250.f; // Distance in world units.

	// How much smaller than the Blueprint's default mesh size to show the player, applied in
	// BeginPlay. The city and its walls are large; a smaller player reads better against them
	// and makes the walls look like the solid barriers they are meant to be.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement") // Editable in the editor and Blueprints.
	float PlayerVisualScale = 0.65f; // 65% of the original size.

	// Multiplies the walking speed the Blueprint set up, applied once in BeginPlay, so the
	// player can cross the much larger city quickly.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement") // Editable in the editor and Blueprints.
	float WalkSpeedMultiplier = 2.f; // Twice as fast as the original.

	// World directions the W/S and D/A keys move along. The camera never rotates, so these
	// are fixed screen directions: W is up the screen (+X) and D is right (+Y).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement") // Editable in the editor and Blueprints.
	FVector ForwardInputDirection = FVector(1.f, 0.f, 0.f); // W moves along world +X.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement") // Editable in the editor and Blueprints.
	FVector RightInputDirection = FVector(0.f, 1.f, 0.f); // D moves along world +Y.

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override; // Shows the cursor, shrinks the player and sets the speed.

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override; // Currently does nothing extra.

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override; // Connects the keys to the functions below.

	// Gives other classes access to the audio component.
	UZeroDayAudioComponent* GetAudioComponent() const { return Audio; } // Walls, threats and stations use this to play sounds.

protected:
	// Holds the camera above and behind the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera") // Visible in the editor.
	TObjectPtr<USpringArmComponent> CameraBoom; // The arm the camera sits on.

	// The top-down view.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera") // Visible in the editor.
	TObjectPtr<UCameraComponent> TopDownCamera; // The camera.

	// Insta Scan (Q) and Validate (F).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Detection") // Visible in the editor.
	TObjectPtr<UDetectionComponent> Detection; // The scan/validate tools.

	// Plays the game's sound effects.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Audio") // Visible in the editor.
	TObjectPtr<UZeroDayAudioComponent> Audio; // The sound effect player.
private:
	// Input handlers, bound in SetupPlayerInputComponent.
	void MoveForward(float Value); // W / S.
	void MoveRight(float Value); // D / A.
	void Fire(); // Left mouse button.
	void Interact(); // E.
	void RestartLevel(); // R.
	void NextLevel(); // N.

	// World position under the mouse cursor, on the player's ground plane.
	FVector GetGroundAimPoint() const; // Used to aim shots.
};
