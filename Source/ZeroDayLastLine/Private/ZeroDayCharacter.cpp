// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayCharacter.h"
#include "ZeroDayProjectile.h"
#include "WallActor.h"
#include "CityTaskActor.h"
#include "DetectionComponent.h"
#include "ZeroDayGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// Sets default values
AZeroDayCharacter::AZeroDayCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Default to the shipped projectile so firing works without Blueprint setup.
	ProjectileClass = AZeroDayProjectile::StaticClass();

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	// Top-down camera on a boom that doesn't collide with the level.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetUsingAbsoluteRotation(true); // Keep the camera fixed in the world. Without this it turns whenever the character turns to face its movement.
	CameraBoom->SetRelativeRotation(FRotator(-70.f, 0.f, 0.f)); // With absolute rotation this is a fixed world angle: looking down, screen-up is world +X.
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// Insta Scan and Validate live in their own component.
	Detection = CreateDefaultSubobject<UDetectionComponent>(TEXT("Detection"));
}

// Called when the game starts or when spawned
void AZeroDayCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Top-down aiming needs a visible, freely-moving cursor. The project's default
	// viewport mouse mode captures/hides it, which would make Fire()'s aim point
	// impossible to see.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

// Called every frame
void AZeroDayCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AZeroDayCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AZeroDayCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AZeroDayCharacter::MoveRight);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AZeroDayCharacter::Fire);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AZeroDayCharacter::Interact);
	PlayerInputComponent->BindAction("Scan", IE_Pressed, Detection.Get(), &UDetectionComponent::PerformScan);
	PlayerInputComponent->BindAction("Validate", IE_Pressed, Detection.Get(), &UDetectionComponent::PerformValidate);
	PlayerInputComponent->BindAction("Restart", IE_Pressed, this, &AZeroDayCharacter::RestartLevel);
	PlayerInputComponent->BindAction("NextLevel", IE_Pressed, this, &AZeroDayCharacter::NextLevel);
}

// W/S. The world direction comes from ForwardInputDirection so it can match the level's orientation.
void AZeroDayCharacter::MoveForward(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(ForwardInputDirection, Value);
	}
}

// A/D. The world direction comes from RightInputDirection.
void AZeroDayCharacter::MoveRight(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(RightInputDirection, Value);
	}
}

// Falls back to a point in front of the player if the cursor ray can't be resolved.
FVector AZeroDayCharacter::GetGroundAimPoint() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) && !FMath::IsNearlyZero(RayDirection.Z))
		{
			// Intersect the cursor ray with the horizontal plane at the player's height.
			// The 2D sprite map has no collision, so a line trace would never hit it.
			const float DistanceAlongRay = (GetActorLocation().Z - RayOrigin.Z) / RayDirection.Z;
			if (DistanceAlongRay > 0.f)
			{
				return RayOrigin + RayDirection * DistanceAlongRay;
			}
		}
	}
	return GetActorLocation() + GetActorForwardVector() * 500.f;
}

// Fires a projectile from the player towards the mouse cursor, flat on the ground plane.
void AZeroDayCharacter::Fire()
{
	if (!ProjectileClass)
	{
		return;
	}

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	FVector AimDirection = (GetGroundAimPoint() - Start).GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AZeroDayProjectile>(ProjectileClass, Start, AimDirection.Rotation(), SpawnParams);
}

// E: fixes the first wall in range, otherwise attends the first city task in range.
void AZeroDayCharacter::Interact()
{
	TArray<AActor*> NearbyWalls;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWallActor::StaticClass(), NearbyWalls);
	for (AActor* Actor : NearbyWalls)
	{
		AWallActor* Wall = Cast<AWallActor>(Actor);
		if (Wall && FVector::Dist(Wall->GetActorLocation(), GetActorLocation()) <= InteractRange)
		{
			Wall->AttemptFix();
			return;
		}
	}

	TArray<AActor*> NearbyTasks;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACityTaskActor::StaticClass(), NearbyTasks);
	for (AActor* Actor : NearbyTasks)
	{
		ACityTaskActor* Task = Cast<ACityTaskActor>(Actor);
		if (Task && FVector::Dist(Task->GetActorLocation(), GetActorLocation()) <= InteractRange)
		{
			Task->Attend();
			return;
		}
	}
}

// R: reload the current level.
void AZeroDayCharacter::RestartLevel()
{
	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->ReloadLevel();
	}
}

// N: advance to the next level (only does anything after a win).
void AZeroDayCharacter::NextLevel()
{
	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->AdvanceToNextLevel(); // Move on to the next level (only works after a win).
	}
}
