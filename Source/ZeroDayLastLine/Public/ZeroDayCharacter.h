// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZeroDayCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AZeroDayProjectile;
class UDetectionComponent;

UCLASS()
class ZERODAYLASTLINE_API AZeroDayCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZeroDayCharacter();

	// What the player fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat")
	TSubclassOf<AZeroDayProjectile> ProjectileClass;

	// How close the player must be to a wall or task to use it with E.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat")
	float InteractRange = 250.f;

	// World directions the W/S and D/A keys move along. Defaults match the level's
	// orientation; change these if the camera or map is rotated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement")
	FVector ForwardInputDirection = FVector(0.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Movement")
	FVector RightInputDirection = FVector(-1.f, 0.f, 0.f);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Holds the camera above and behind the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// The top-down view.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	// Insta Scan (Q) and Validate (F).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Detection")
	TObjectPtr<UDetectionComponent> Detection;

private:
	// Input handlers, bound in SetupPlayerInputComponent.
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Fire();
	void Interact();
	void RestartLevel();
	void NextLevel();

	// World position under the mouse cursor, on the player's ground plane.
	FVector GetGroundAimPoint() const;
};
