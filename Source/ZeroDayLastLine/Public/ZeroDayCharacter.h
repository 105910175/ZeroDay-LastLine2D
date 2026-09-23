// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZeroDayCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AZeroDayProjectile;

UCLASS()
class ZERODAYLASTLINE_API AZeroDayCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZeroDayCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat")
	TSubclassOf<AZeroDayProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZeroDay|Combat")
	float InteractRange = 250.f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZeroDay|Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Fire();
	void Interact();

	FVector GetGroundAimPoint() const;
};
