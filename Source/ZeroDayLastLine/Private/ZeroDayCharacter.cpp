// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayCharacter.h" // This class's own declaration.
#include "ZeroDayProjectile.h" // What the player fires.
#include "WallActor.h" // Walls the player can repair.
#include "CityTaskActor.h" // Stations the player can attend.
#include "DetectionComponent.h" // Insta Scan and Validate.
#include "Audio/ZeroDayAudioComponent.h" // The sound effect player.

#include "ZeroDayGameMode.h" // Restart / next level go through the game mode.
#include "GameFramework/SpringArmComponent.h" // Holds the camera away from the player.
#include "Camera/CameraComponent.h" // The top-down view.
#include "GameFramework/CharacterMovementComponent.h" // Walking speed and turning.
#include "Components/CapsuleComponent.h" // The collision capsule.
#include "Kismet/GameplayStatics.h" // Helpers such as GetAllActorsOfClass.
#include "GameFramework/PlayerController.h" // The mouse cursor and input mode.

// Sets default values
AZeroDayCharacter::AZeroDayCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true; // Kept on (Tick is empty) so it is easy to add per-frame logic later.

	// Default to the shipped projectile so firing works without Blueprint setup.
	ProjectileClass = AZeroDayProjectile::StaticClass(); // The plasma bolt.

	bUseControllerRotationYaw = false; // The character turns by itself, not with the mouse.

	GetCharacterMovement()->bOrientRotationToMovement = true; // Face the way we are walking.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f); // Turn quickly (720 degrees a second).

	// Top-down camera on a boom that doesn't collide with the level.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // The arm the camera sits on.
	CameraBoom->SetupAttachment(RootComponent); // It follows the player.
	CameraBoom->TargetArmLength = 1650.f; // Pulled back further so the larger city (bigger walls, more room to walk) still fits on screen.
	CameraBoom->SetUsingAbsoluteRotation(true); // Keep the camera fixed in the world. Without this it turns whenever the character turns to face its movement.
	CameraBoom->SetRelativeRotation(FRotator(-70.f, 0.f, 0.f)); // With absolute rotation this is a fixed world angle: looking down, screen-up is world +X.
	CameraBoom->bDoCollisionTest = false; // Never let level geometry push the camera in.
	CameraBoom->bUsePawnControlRotation = false; // The mouse does not rotate the camera.

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera")); // The camera itself.
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Mount it on the end of the boom.
	TopDownCamera->bUsePawnControlRotation = false; // It keeps the boom's fixed angle.

	// Insta Scan and Validate live in their own component.
	Detection = CreateDefaultSubobject<UDetectionComponent>(TEXT("Detection")); // The scan/validate tools.
	Audio = CreateDefaultSubobject<UZeroDayAudioComponent>(TEXT("Audio")); // The sound effect player.
}

// Called when the game starts or when spawned
void AZeroDayCharacter::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	// Top-down aiming needs a visible, freely-moving cursor. The project's default
	// viewport mouse mode captures/hides it, which would make Fire()'s aim point
	// impossible to see.
	if (APlayerController* PC = Cast<APlayerController>(GetController())) // The player's controller.
	{
		PC->bShowMouseCursor = true; // Show the cursor.
		PC->SetInputMode(FInputModeGameAndUI()); // Let it move freely while the game still gets keys.
	}

	// Shrink the player relative to whatever mesh the Blueprint set up, so they read as
	// human-sized against the much larger walls and stations. Done here (rather than the
	// constructor) so it applies no matter what the Blueprint's construction script sets.
	if (USkeletalMeshComponent* CharacterMesh = GetMesh()) // The visible character.
	{
		const FVector OriginalMeshLocation = CharacterMesh->GetRelativeLocation(); // The Blueprint's own ground-alignment offset (usually a negative Z so the feet touch the floor).
		CharacterMesh->SetRelativeScale3D(FVector(PlayerVisualScale)); // Shrink the visible mesh uniformly.
		CharacterMesh->SetRelativeLocation(FVector(OriginalMeshLocation.X, OriginalMeshLocation.Y, OriginalMeshLocation.Z * PlayerVisualScale)); // Scale the ground offset too, so the feet still meet the floor instead of floating or sinking.
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent()) // The collision capsule.
	{
		Capsule->SetCapsuleSize(Capsule->GetUnscaledCapsuleRadius() * PlayerVisualScale, Capsule->GetUnscaledCapsuleHalfHeight() * PlayerVisualScale); // Shrink collision to match, so the smaller player doesn't still bump things at the old size.
	}

	// Walk faster, so crossing the large city between a station and a wall does not feel slow.
	// Multiplying (rather than setting) keeps whatever speed the Blueprint chose as the base.
	GetCharacterMovement()->MaxWalkSpeed *= WalkSpeedMultiplier; // 2x the original speed.
}

// Called every frame
void AZeroDayCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // Nothing extra to do per frame yet.

}

// Called to bind functionality to input
void AZeroDayCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent); // Let the base class bind its defaults first.

	PlayerInputComponent->BindAxis("MoveForward", this, &AZeroDayCharacter::MoveForward); // W / S.
	PlayerInputComponent->BindAxis("MoveRight", this, &AZeroDayCharacter::MoveRight); // D / A.
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AZeroDayCharacter::Fire); // Left mouse button.
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AZeroDayCharacter::Interact); // E.
	PlayerInputComponent->BindAction("Scan", IE_Pressed, Detection.Get(), &UDetectionComponent::PerformScan); // Q: Insta Scan.
	PlayerInputComponent->BindAction("Validate", IE_Pressed, Detection.Get(), &UDetectionComponent::PerformValidate); // F: Validate.
	PlayerInputComponent->BindAction("Restart", IE_Pressed, this, &AZeroDayCharacter::RestartLevel); // R.
	PlayerInputComponent->BindAction("NextLevel", IE_Pressed, this, &AZeroDayCharacter::NextLevel); // N.
}

// W/S. The world direction comes from ForwardInputDirection so it can match the level's orientation.
void AZeroDayCharacter::MoveForward(float Value)
{
	if (Value != 0.f) // Only when a key is held.
	{
		AddMovementInput(ForwardInputDirection, Value); // +1 for W, -1 for S.
	}
}

// A/D. The world direction comes from RightInputDirection.
void AZeroDayCharacter::MoveRight(float Value)
{
	if (Value != 0.f) // Only when a key is held.
	{
		AddMovementInput(RightInputDirection, Value); // +1 for D, -1 for A.
	}
}

// Falls back to a point in front of the player if the cursor ray can't be resolved.
FVector AZeroDayCharacter::GetGroundAimPoint() const
{
	if (APlayerController* PC = Cast<APlayerController>(GetController())) // The player's controller.
	{
		FVector RayOrigin; // Where the cursor's ray starts (at the camera).
		FVector RayDirection; // Which way the ray points.
		if (PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) && !FMath::IsNearlyZero(RayDirection.Z)) // Turn the cursor into a world ray that is not parallel to the ground.
		{
			// Intersect the cursor ray with the horizontal plane at the player's height.
			// The 2D sprite map has no collision, so a line trace would never hit it.
			const float DistanceAlongRay = (GetActorLocation().Z - RayOrigin.Z) / RayDirection.Z; // How far along the ray the ground plane is.
			if (DistanceAlongRay > 0.f) // Only in front of the camera.
			{
				return RayOrigin + RayDirection * DistanceAlongRay; // The point on the ground under the cursor.
			}
		}
	}
	return GetActorLocation() + GetActorForwardVector() * 500.f; // Fallback: straight ahead.
}

// Fires a projectile from the player towards the mouse cursor, flat on the ground plane.
void AZeroDayCharacter::Fire()
{
	if (!ProjectileClass) // Nothing to fire.
	{
		return; // Bail out.
	}

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 50.f); // Start a little above the feet.
	FVector AimDirection = (GetGroundAimPoint() - Start).GetSafeNormal2D(); // Flat direction towards the cursor.
	if (AimDirection.IsNearlyZero()) // Cursor right on top of the player.
	{
		AimDirection = GetActorForwardVector(); // Fire the way we face.
	}
	if (Audio) // If we have a sound player...
	{
		Audio->PlayFire(GetActorLocation()); // ...play the firing sound.
	}

	FActorSpawnParameters SpawnParams; // Settings for the spawn call.
	SpawnParams.Owner = this; // The shot remembers who fired it (used for sounds).
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // Always appear, even if overlapping.

	GetWorld()->SpawnActor<AZeroDayProjectile>(ProjectileClass, Start, AimDirection.Rotation(), SpawnParams); // Create the shot.
}

// E: fixes the first wall in range, otherwise attends the first city task in range.
void AZeroDayCharacter::Interact()
{
	TArray<AActor*> NearbyWalls; // Every wall in the world.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AWallActor::StaticClass(), NearbyWalls); // Fill the list.
	for (AActor* Actor : NearbyWalls) // Check each wall.
	{
		AWallActor* Wall = Cast<AWallActor>(Actor); // Treat it as a wall.
		if (Wall && Wall->bIsActiveThisLevel && Wall->GetStatus() == EWallStatus::Breached && Wall->GetDistanceToPoint(GetActorLocation()) <= InteractRange) // A wall in play, actually broken, and close (nearest point on the wall, not its possibly distant centre).
		{
			Wall->AttemptFix(); // Try to patch it.
			return; // One action per press.
		}
	}

	TArray<AActor*> NearbyTasks; // Every station in the world.
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACityTaskActor::StaticClass(), NearbyTasks); // Fill the list.
	for (AActor* Actor : NearbyTasks) // Check each station.
	{
		ACityTaskActor* Task = Cast<ACityTaskActor>(Actor); // Treat it as a station.
		if (Task && Task->bIsActiveThisLevel && FVector::Dist(Task->GetActorLocation(), GetActorLocation()) <= InteractRange + Task->GetInteractRadius()) // In play and close enough to use (measured from the station's edge, since stations are now big)?
		{
			Task->Attend(); // Try to attend it.
			return; // One action per press.
		}
	}
}

// R: reload the current level.
void AZeroDayCharacter::RestartLevel()
{
	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
	{
		GameMode->ReloadLevel(); // Start the current level over.
	}
}

// N: advance to the next level (only does anything after a win).
void AZeroDayCharacter::NextLevel()
{
	if (AZeroDayGameMode* GameMode = Cast<AZeroDayGameMode>(UGameplayStatics::GetGameMode(this))) // Find our game mode.
	{
		GameMode->AdvanceToNextLevel(); // Move on to the next level (only works after a win).
	}
}
