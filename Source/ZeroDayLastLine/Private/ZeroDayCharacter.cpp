// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayCharacter.h"
#include "ZeroDayProjectile.h"
#include "WallActor.h"
#include "CityTaskActor.h"
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

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetRelativeRotation(FRotator(-70.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
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
}

void AZeroDayCharacter::MoveForward(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void AZeroDayCharacter::MoveRight(float Value)
{
	if (Value != 0.f)
	{
		AddMovementInput(FVector::RightVector, Value);
	}
}

FVector AZeroDayCharacter::GetGroundAimPoint() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FHitResult Hit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			return Hit.Location;
		}
	}
	return GetActorLocation() + GetActorForwardVector() * 500.f;
}

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
