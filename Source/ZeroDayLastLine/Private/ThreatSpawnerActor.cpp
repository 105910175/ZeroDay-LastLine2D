#include "ThreatSpawnerActor.h" // This class's own declaration.
#include "ThreatPawn.h" // The threats we spawn.
#include "WallActor.h" // The wall they walk towards.
#include "CooldownComponent.h" // The spawn timer.
#include "ZeroDayLevelConfig.h" // The level rules we obey.
#include "Components/SceneComponent.h" // A plain transform to use as the root.
#include "Kismet/GameplayStatics.h"

// Just a transform in the level plus a timer; spawned threats appear at this actor's location.
AThreatSpawnerActor::AThreatSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false; // The timer component does the timing, not us.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); // An invisible root so the actor has a location.

	// Default to the shipped threat so a placed spawner works without Blueprint setup.
	ThreatClass = AThreatPawn::StaticClass(); // Spawn our own threat class by default.

	SpawnTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("SpawnTimer")); // The shared looping timer.
}

// Starts spawning immediately; the game mode can pause it for inactive walls or when the game ends.
void AThreatSpawnerActor::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	SpawnTimer->OnExpired.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnTick); // Call HandleSpawnTick each time the timer fires.
	SetSpawningEnabled(true); // Begin the looping timer.
}

// Starts a looping timer, or cancels it to stop spawning.
void AThreatSpawnerActor::SetSpawningEnabled(bool bEnabled)
{
	if (bEnabled) // Turning spawning on?
	{
		SpawnTimer->StartCooldown(SpawnInterval, true); // Start (or restart) the looping timer.
	}
	else
	{
		SpawnTimer->CancelCooldown(); // Stop the timer so nothing more spawns.
	}
}

// Copies the level's difficulty onto this spawner. The caller restarts the timer afterwards.
void AThreatSpawnerActor::ApplyLevelConfig(const FZeroDayLevelConfig& Config)
{
	SpawnInterval = Config.SpawnInterval; // How often to spawn.
	MaxConcurrentThreats = Config.MaxConcurrentThreats; // How many can be alive at once.
	DecoyChance = Config.DecoyChance; // How many are false positives.
	CriticalChance = Config.CriticalChance; // How many are critical.
	MediumChance = Config.MediumChance; // How many are medium.
	ThreatSpeedScale = Config.ThreatSpeedScale; // How fast they walk.
	ThreatHealthScale = Config.ThreatHealthScale; // How tough they are.
}

// Spawn one threat per tick, unless misconfigured, the wall is out of play, or the cap is reached.
void AThreatSpawnerActor::HandleSpawnTick()
{
	if (!ThreatClass || !TargetWall || !TargetWall->bIsActiveThisLevel) // Missing setup or the wall is not in play.
	{
		return; // Nothing to spawn.
	}

	if (LiveThreatCount >= MaxConcurrentThreats) // Already at the cap?
	{
		return; // Wait for some to die.
	}

	// Roll severity: Critical first, then Medium, otherwise Low.
	const float SeverityRoll = FMath::FRand(); // Random number from 0 to 1.
	EThreatSeverity Severity = EThreatSeverity::Low; // Default to the weakest.
	if (SeverityRoll < CriticalChance) // Lowest slice of the roll is critical.
	{
		Severity = EThreatSeverity::Critical; // A dangerous one.
	}
	else if (SeverityRoll < CriticalChance + MediumChance) // Next slice is medium.
	{
		Severity = EThreatSeverity::Medium; // A middling one.
	}

	SpawnThreat(Severity, FMath::FRand() < DecoyChance); // Spawn it; decoy chance decides if it is harmless.
}

// Spawns one threat with an exact severity and decoy flag.
bool AThreatSpawnerActor::SpawnThreat(EThreatSeverity Severity, bool bDecoy)
{
	if (!ThreatClass || !TargetWall || !GetWorld()) // Cannot spawn without these.
	{
		return false; // Report failure.
	}

    FActorSpawnParameters SpawnParams; // Settings for the spawn call.
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; // Always appear, nudged if something is in the way.
    SpawnParams.Instigator = UGameplayStatics::GetPlayerPawn(this, 0); // Remember which player caused this threat to be spawned.

	// Scatter spawn points a little so a group of threats doesn't stack in one spot.
	const FVector Scatter(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0.f); // Random ground-plane offset.
	AThreatPawn* NewThreat = GetWorld()->SpawnActor<AThreatPawn>(ThreatClass, GetActorLocation() + Scatter, GetActorRotation(), SpawnParams); // Create the threat.
	if (!NewThreat) // The engine refused to spawn it.
	{
		return false; // Report failure.
	}

	NewThreat->InitialiseThreat(TargetWall, Severity, bDecoy, ThreatSpeedScale, ThreatHealthScale); // Give it its target, stats and identity.
	NewThreat->OnDestroyed.AddDynamic(this, &AThreatSpawnerActor::HandleSpawnedThreatDestroyed); // Track when it dies so the concurrency cap stays accurate.
	++LiveThreatCount; // One more threat alive.
	return true; // Success.
}

// Zero-day surge: a burst of real threats that ignores the concurrency cap.
void AThreatSpawnerActor::SpawnBurst(int32 Count, EThreatSeverity Severity)
{
	for (int32 Index = 0; Index < Count; ++Index) // Spawn the requested number.
	{
		SpawnThreat(Severity, false); // Surge threats are always real.
	}
}

// Fires whether the threat was shot or reached the wall.
void AThreatSpawnerActor::HandleSpawnedThreatDestroyed(AActor* DestroyedActor)
{
	LiveThreatCount = FMath::Max(0, LiveThreatCount - 1); // One fewer alive (never below zero).
}
