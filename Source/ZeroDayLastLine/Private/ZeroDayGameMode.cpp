// Fill out your copyright notice in the Description page of Project Settings.

#include "ZeroDayGameMode.h" // This class's own declaration.
#include "WallActor.h" // The city walls.
#include "CityTaskActor.h" // The city stations.
#include "ThreatSpawnerActor.h" // The threat spawners.
#include "ThreatPawn.h" // The threats (cleared between levels).
#include "CooldownComponent.h" // The reusable timer used for the level and surges.
#include "ZeroDayTypes.h" // Shared enums and name helpers.
#include "ZeroDayCharacter.h" // The player (default pawn and movement directions).
#include "ZeroDayHUD.h" // The HUD class this mode uses.
#include "EngineUtils.h" // TActorIterator for finding actors of a type.
#include "Kismet/GameplayStatics.h" // Helpers such as GetPlayerPawn.

// Use our character and HUD by default, and create the two timers.
AZeroDayGameMode::AZeroDayGameMode()
{
	DefaultPawnClass = AZeroDayCharacter::StaticClass(); // Spawn our top-down character for the player.
	HUDClass = AZeroDayHUD::StaticClass(); // Draw our HUD.

	LevelTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("LevelTimer")); // Counts down the level.
	SurgeTimer = CreateDefaultSubobject<UCooldownComponent>(TEXT("SurgeTimer")); // Triggers surge waves.
}

// Find everything placed in the level (or build a default city), subscribe to events, then start the level.
void AZeroDayGameMode::BeginPlay()
{
	Super::BeginPlay(); // Let the base class start up first.

	for (TActorIterator<AWallActor> It(GetWorld()); It; ++It) // Every wall already in the map.
	{
		RegisterWall(*It); // Track it and listen to its events.
	}
	for (TActorIterator<ACityTaskActor> It(GetWorld()); It; ++It) // Every station already in the map.
	{
		RegisterTask(*It); // Track it and listen to its events.
	}
	for (TActorIterator<AThreatSpawnerActor> It(GetWorld()); It; ++It) // Every spawner already in the map.
	{
		RegisterSpawner(*It); // Track it.
	}

	if (Walls.Num() == 0 && bAutoBuildLayout) // The map has no city, so make one.
	{
		BuildDefaultLayout(); // Spawn walls, spawners and stations around the player.
	}

	LevelTimer->OnExpired.AddDynamic(this, &AZeroDayGameMode::HandleLevelSurvived); // Timer ends -> the player wins.
	SurgeTimer->OnExpired.AddDynamic(this, &AZeroDayGameMode::HandleSurge); // Surge timer fires -> send a wave.

	StartLevel(CurrentLevel); // Begin at whichever level is selected (level 1 by default).
}

// Track a wall once and listen for its status changes and loss.
void AZeroDayGameMode::RegisterWall(AWallActor* Wall)
{
	if (!Wall || Walls.Contains(Wall)) // Ignore null or already-known walls.
	{
		return; // Nothing to do.
	}
	Walls.Add(Wall); // Remember it.
	Wall->OnWallLost.AddDynamic(this, &AZeroDayGameMode::HandleWallLost); // Wall lost -> lose the level.
	Wall->OnStatusUpdated.AddDynamic(this, &AZeroDayGameMode::HandleWallStatusChanged); // Status change -> show an alert.
}

// Track a station once and listen for it failing.
void AZeroDayGameMode::RegisterTask(ACityTaskActor* Task)
{
	if (!Task || Tasks.Contains(Task)) // Ignore null or already-known stations.
	{
		return; // Nothing to do.
	}
	Tasks.Add(Task); // Remember it.
	Task->OnTaskFailed.AddDynamic(this, &AZeroDayGameMode::HandleTaskFailed); // Station failed -> lose the level.
}

// Track a spawner once.
void AZeroDayGameMode::RegisterSpawner(AThreatSpawnerActor* Spawner)
{
	if (Spawner) // Ignore null.
	{
		Spawners.AddUnique(Spawner); // Remember it (no duplicates).
	}
}

// Builds a default city around the player: four walls, a spawner outside each, and three stations inside.
void AZeroDayGameMode::BuildDefaultLayout()
{
	UWorld* World = GetWorld(); // The world to spawn into.
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); // The player is the centre of the city.
	const FVector Centre = Player ? Player->GetActorLocation() : FVector::ZeroVector; // Use the origin if there is no player yet.

	// "North" is whichever way the W key moves the player, and "East" is the D key's direction.
	FVector Forward(0.f, 1.f, 0.f); // Fallback: world +Y.
	FVector Right(-1.f, 0.f, 0.f); // Fallback: world -X.
	if (const AZeroDayCharacter* Character = Cast<AZeroDayCharacter>(Player)) // If the player is our character...
	{
		Forward = Character->ForwardInputDirection.GetSafeNormal(); // ...use its real forward direction.
		Right = Character->RightInputDirection.GetSafeNormal(); // ...and its real right direction.
	}

	FActorSpawnParameters SpawnParams; // Settings shared by every spawn below.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // Always appear, even if overlapping.

	// Describe each side once so the four walls are created by one loop.
	struct FSideSetup
	{
		EWallSide Side; // Which wall this is.
		FVector Direction; // Which way it lies from the centre.
		float Distance; // How far from the centre.
		float WallLength; // How long the wall is along the edge of the city.
	};
	constexpr float WallBaseLength = 300.f; // A wall is 300 units long before scaling (the mesh is 100 x 3).
	constexpr float WallCoverage = 0.8f; // Each wall covers 80% of its side of the city, leaving gaps at the corners.
	const FSideSetup Sides[] = {
		{ EWallSide::North, Forward, CityHalfExtents.Y, 2.f * CityHalfExtents.X * WallCoverage }, // Front wall spans the width.
		{ EWallSide::South, -Forward, CityHalfExtents.Y, 2.f * CityHalfExtents.X * WallCoverage }, // Back wall spans the width.
		{ EWallSide::East, Right, CityHalfExtents.X, 2.f * CityHalfExtents.Y * WallCoverage }, // Right wall spans the depth.
		{ EWallSide::West, -Right, CityHalfExtents.X, 2.f * CityHalfExtents.Y * WallCoverage }, // Left wall spans the depth.
	};

	for (const FSideSetup& Setup : Sides) // Create a wall and spawner for each side.
	{
		const FVector WallPosition = Centre + Setup.Direction * Setup.Distance; // Where the wall stands.
		const FRotator WallRotation(0.f, Setup.Direction.Rotation().Yaw + 90.f, 0.f); // Turn it so its long side faces the centre.
		const FTransform WallTransform(WallRotation, WallPosition, FVector(Setup.WallLength / WallBaseLength, 1.f, 1.f)); // Stretch only along the wall's long axis.
		AWallActor* Wall = World->SpawnActor<AWallActor>(AWallActor::StaticClass(), WallTransform, SpawnParams); // Build the wall.
		if (!Wall) // Spawn failed.
		{
			continue; // Skip this side.
		}
		Wall->WallSide = Setup.Side; // Tell the wall which side it guards.
		RegisterWall(Wall); // Track it.

		const FVector SpawnerPosition = Centre + Setup.Direction * (Setup.Distance + SpawnerDistanceBeyondWall); // Outside the wall.
		AThreatSpawnerActor* Spawner = World->SpawnActor<AThreatSpawnerActor>(AThreatSpawnerActor::StaticClass(), SpawnerPosition, FRotator::ZeroRotator, SpawnParams); // Build the spawner.
		if (Spawner) // Spawn worked.
		{
			Spawner->TargetWall = Wall; // Its threats walk to this wall.
			RegisterSpawner(Spawner); // Track it.
		}
	}

	// Three stations inside the walls, spread out so the player has to run between them.
	struct FStationSetup
	{
		ECityTaskKind Kind; // Which station this is.
		FVector Position; // Where it stands.
	};
	const FStationSetup Stations[] = {
		{ ECityTaskKind::Power, Centre + Right * (-CityHalfExtents.X * 0.45f) + Forward * (CityHalfExtents.Y * 0.30f) }, // Front-left.
		{ ECityTaskKind::Food, Centre + Right * (CityHalfExtents.X * 0.45f) + Forward * (CityHalfExtents.Y * 0.30f) }, // Front-right.
		{ ECityTaskKind::Medic, Centre + Forward * (-CityHalfExtents.Y * 0.40f) }, // Back-middle.
	};
	for (const FStationSetup& Setup : Stations) // Create each station.
	{
		ACityTaskActor* Task = World->SpawnActor<ACityTaskActor>(ACityTaskActor::StaticClass(), Setup.Position, FRotator::ZeroRotator, SpawnParams); // Build it.
		if (Task) // Spawn worked.
		{
			Task->TaskKind = Setup.Kind; // Power, food or medical.
			RegisterTask(Task); // Track it.
		}
	}
}

// Resets everything and begins the given level, applying its difficulty.
void AZeroDayGameMode::StartLevel(int32 Level)
{
	CurrentLevel = FMath::Clamp(Level, 1, FZeroDayLevelConfig::MaxLevel); // Keep the level within 1..3.
	Config = FZeroDayLevelConfig::ForLevel(CurrentLevel); // Load this level's rules.

	// Mark the game as ended while resetting so the walls' "secured" status alerts stay quiet.
	bGameEnded = true; // Suppresses alerts during the reset.

	TArray<AThreatPawn*> LeftoverThreats; // Threats from the previous run.
	for (TActorIterator<AThreatPawn> It(GetWorld()); It; ++It) // Find every live threat.
	{
		LeftoverThreats.Add(*It); // Collect first so we don't destroy while iterating.
	}
	for (AThreatPawn* Threat : LeftoverThreats) // Now remove them all.
	{
		Threat->Destroy(); // Gone (spawners and walls are told automatically).
	}

	for (AWallActor* Wall : Walls) // Set up every wall.
	{
		if (!Wall) // Skip anything that has been destroyed.
		{
			continue; // Next wall.
		}
		Wall->ResetForLevel(); // Full health, no countdown.
		Wall->ApplyLevelSettings(Config.RemediationWindowSeconds); // This level's breach time.
		const bool bWallActive = (Config.ActiveWallCount <= 1) ? (Wall->WallSide == EWallSide::North) : true; // Level 1 uses only the north wall.
		Wall->SetActiveThisLevel(bWallActive); // Show/hide it accordingly.
	}

	for (ACityTaskActor* Task : Tasks) // Set up every station.
	{
		if (!Task) // Skip anything that has been destroyed.
		{
			continue; // Next station.
		}
		Task->ResetForLevel(Config.TaskDecayScale); // Full meter at this level's decay speed.
		Task->SetActiveThisLevel(Config.bTasksActive); // Only running on the levels that use stations.
	}

	for (AThreatSpawnerActor* Spawner : Spawners) // Set up every spawner.
	{
		if (!Spawner) // Skip anything that has been destroyed.
		{
			continue; // Next spawner.
		}
		Spawner->ApplyLevelConfig(Config); // Interval, caps, chances and scaling for this level.
		Spawner->SetSpawningEnabled(true); // Restart its timer with the new interval.
	}

	Inventory[static_cast<int32>(EResourceType::Intel)] = Config.StartingIntel; // Fresh Intel.
	Inventory[static_cast<int32>(EResourceType::Patch)] = Config.StartingPatch; // Fresh Patch kits.
	Inventory[static_cast<int32>(EResourceType::Food)] = Config.StartingFood; // Fresh Food.
	Inventory[static_cast<int32>(EResourceType::Power)] = Config.StartingPower; // Fresh Power cells.
	Inventory[static_cast<int32>(EResourceType::Medical)] = Config.StartingMedical; // Fresh Medical kits.
	Survivors = Config.StartingSurvivors; // Everyone is alive at the start.
	MaxSurvivors = Config.StartingSurvivors; // Remember how many there were for the HUD.
	Stats = FZeroDayLevelStats(); // Clear the previous level's results.
	Effects.Empty(); // Clear any rings still animating.
	Alerts.Empty(); // Clear the previous level's messages.
	EndMessage.Empty(); // No result yet.
	bPlayerWon = false; // Not won yet.
	LevelStartTime = GetWorld()->GetTimeSeconds(); // Remember when this level began (for the banner).

	LevelTimer->CancelCooldown(); // Drop any old countdown...
	LevelTimer->StartCooldown(Config.DurationSeconds, false); // ...and start this level's one-shot countdown.

	SurgeTimer->CancelCooldown(); // Drop any old surge timer.
	if (Config.bSurgesEnabled) // Only some levels have surges.
	{
		SurgeTimer->StartCooldown(Config.SurgeIntervalSeconds, true); // Looping timer for repeat surges.
	}

	bGameEnded = false; // The level is live again.
	PostAlert(Config.Title, FColor::Cyan); // Announce which level just started.
}

// True after winning a level that has a following level.
bool AZeroDayGameMode::CanAdvanceLevel() const
{
	return bGameEnded && bPlayerWon && CurrentLevel < FZeroDayLevelConfig::MaxLevel; // Won, and not the last level.
}

// True after winning the final level.
bool AZeroDayGameMode::IsFinalLevelComplete() const
{
	return bGameEnded && bPlayerWon && CurrentLevel >= FZeroDayLevelConfig::MaxLevel; // Won the last level.
}

// R: start the current level over.
void AZeroDayGameMode::ReloadLevel()
{
	StartLevel(CurrentLevel); // Reset and replay the same level.
}

// N: move on after a win.
void AZeroDayGameMode::AdvanceToNextLevel()
{
	if (CanAdvanceLevel()) // Only allowed after winning a level that has a next one.
	{
		StartLevel(CurrentLevel + 1); // Begin the next level.
	}
}

// How many of a resource the player is carrying.
int32 AZeroDayGameMode::GetResource(EResourceType Type) const
{
	return Inventory[static_cast<int32>(Type)]; // Look it up by its enum index.
}

// True if the player has at least Amount of the resource.
bool AZeroDayGameMode::CanAfford(EResourceType Type, int32 Amount) const
{
	return GetResource(Type) >= Amount; // Enough in the inventory?
}

// Adds resources, for example from a pickup.
void AZeroDayGameMode::AddResource(EResourceType Type, int32 Amount)
{
	int32& Slot = Inventory[static_cast<int32>(Type)]; // The counter for this resource.
	Slot = FMath::Max(0, Slot + Amount); // Add, but never go below zero.
}

// All-or-nothing: never spends part of the cost.
bool AZeroDayGameMode::SpendResource(EResourceType Type, int32 Amount)
{
	if (!CanAfford(Type, Amount)) // Can the player afford it?
	{
		return false; // No, spend nothing.
	}
	Inventory[static_cast<int32>(Type)] -= Amount; // Yes, pay the full cost.
	return true; // Tell the caller it worked.
}

// Queue a message for the HUD, keeping only the most recent few.
void AZeroDayGameMode::PostAlert(const FString& Message, const FColor& Color)
{
	constexpr int32 MaxAlerts = 6; // Never show more than six messages.
	constexpr float AlertLifetimeSeconds = 6.f; // Each message stays for six seconds.

	FZeroDayAlert Alert; // Build the new message.
	Alert.Message = Message; // Its text.
	Alert.Color = Color; // Its colour.
	Alert.ExpireTime = GetWorld()->GetTimeSeconds() + AlertLifetimeSeconds; // When it should vanish.
	Alerts.Add(Alert); // Add it to the list.

	if (Alerts.Num() > MaxAlerts) // Too many messages on screen?
	{
		Alerts.RemoveAt(0); // Drop the oldest.
	}
}

// Queues a ring for the HUD to draw, dropping any that have already finished.
void AZeroDayGameMode::PostEffect(const FVector& Location, const FLinearColor& Color, float MaxRadius, float Duration)
{
	const float Now = GetWorld()->GetTimeSeconds(); // Current world time.
	Effects.RemoveAll([Now](const FZeroDayEffect& Old) { return Now > Old.StartTime + Old.Duration; }); // Forget finished rings.

	FZeroDayEffect Effect; // Build the new ring.
	Effect.Location = Location; // Where it appears.
	Effect.Color = Color; // What colour it is.
	Effect.MaxRadius = MaxRadius; // How big it grows.
	Effect.StartTime = Now; // When it began.
	Effect.Duration = Duration; // How long it lasts.
	Effects.Add(Effect); // Queue it for drawing.
}

// A threat was shot down: count real ones and show a small burst where it fell.
void AZeroDayGameMode::OnThreatKilled(const AThreatPawn* Threat)
{
	if (!Threat || bGameEnded) // Nothing to record, or the level is over.
	{
		return; // Ignore.
	}
	if (!Threat->bIsDecoy) // False positives don't count as a real success.
	{
		++Stats.ThreatsStopped; // Count the real threat.
	}
	PostEffect(Threat->GetActorLocation(), FLinearColor(1.f, 0.75f, 0.3f), 70.f, 0.4f); // Same colour for everyone so it doesn't reveal severity.
}

// Seconds left to survive, for the HUD.
float AZeroDayGameMode::GetLevelTimeRemaining() const
{
	return LevelTimer->GetTimeRemaining(); // Read the level countdown.
}

// Seconds since this level began.
float AZeroDayGameMode::GetTimeSinceLevelStart() const
{
	return GetWorld()->GetTimeSeconds() - LevelStartTime; // Now minus the start time.
}

// Turns wall status changes into player-facing alerts.
void AZeroDayGameMode::HandleWallStatusChanged(AWallActor* Wall, EWallStatus NewStatus)
{
	if (!Wall || bGameEnded) // No wall, or the level is over/resetting.
	{
		return; // Stay quiet.
	}

	const TCHAR* Side = WallSideToString(Wall->WallSide); // "North", "East", ...
	switch (NewStatus) // Choose the message by the new status.
	{
	case EWallStatus::Threatened: // Health dropped past the threshold.
		PostAlert(FString::Printf(TEXT("%s wall under attack (integrity %.0f%%)"), Side, Wall->GetHealthPercent() * 100.f), FColor::Orange);
		break; // Done.
	case EWallStatus::Breached: // Health reached zero.
		PostAlert(FString::Printf(TEXT("%s wall BREACHED: fix it with a Patch (E) or clear the threats!"), Side), FColor(255, 105, 90));
		PostEffect(Wall->GetActorLocation(), FLinearColor::Red, 260.f, 0.9f); // Big red ring on the broken wall.
		break; // Done.
	default: // Back to Safe after a repair.
		PostAlert(FString::Printf(TEXT("%s wall secured"), Side), FColor::Green);
		PostEffect(Wall->GetActorLocation(), FLinearColor::Green, 200.f, 0.7f); // Green ring on the repaired wall.
		break; // Done.
	}
}

// A wall stayed breached too long: lose.
void AZeroDayGameMode::HandleWallLost(AWallActor* LostWall)
{
	EndGame(false, FString::Printf(TEXT("The %s wall fell and the zero-day was weaponised."), LostWall ? WallSideToString(LostWall->WallSide) : TEXT("outer"))); // Name the wall in the result.
}

// A city station went unattended too long: a survivor is lost, and the level is lost if none remain.
void AZeroDayGameMode::HandleTaskFailed(ACityTaskActor* FailedTask)
{
	if (bGameEnded) // The level is already over.
	{
		return; // Ignore.
	}

	Survivors = FMath::Max(0, Survivors - 1); // One fewer survivor (never below zero).
	++Stats.SurvivorsLost; // Remember it for the result screen.
	const TCHAR* StationName = FailedTask ? TaskKindToString(FailedTask->TaskKind) : TEXT("A station"); // Which station failed.
	PostAlert(FString::Printf(TEXT("A survivor was lost: the %s station ran out. %d left."), StationName, Survivors), FColor(255, 105, 90)); // Tell the player.
	if (FailedTask) // If we know where it happened...
	{
		PostEffect(FailedTask->GetActorLocation(), FLinearColor::Red, 180.f, 1.0f); // ...mark the spot with a red ring.
	}

	if (Survivors <= 0) // Nobody left?
	{
		EndGame(false, TEXT("The last survivor was lost because the city's stations were left to fail.")); // Lose the level.
	}
}

// The level timer ran out with everything still standing: win.
void AZeroDayGameMode::HandleLevelSurvived()
{
	if (CurrentLevel >= FZeroDayLevelConfig::MaxLevel) // Was that the last level?
	{
		EndGame(true, TEXT("All three levels cleared. The zero-day is contained and the city is safe.")); // Whole game won.
	}
	else
	{
		EndGame(true, FString::Printf(TEXT("Level %d cleared. The city holds."), CurrentLevel)); // Just this level won.
	}
}

// Zero-day surge (level 3): a burst of critical threats at one randomly chosen wall.
void AZeroDayGameMode::HandleSurge()
{
	if (bGameEnded) // Nothing happens after the level is over.
	{
		return; // Ignore.
	}

	TArray<AThreatSpawnerActor*> Candidates; // Spawners whose wall is in play.
	for (AThreatSpawnerActor* Spawner : Spawners) // Check each spawner.
	{
		if (Spawner && Spawner->TargetWall && Spawner->TargetWall->bIsActiveThisLevel) // Has an active wall to attack.
		{
			Candidates.Add(Spawner); // It can be used for the surge.
		}
	}
	if (Candidates.Num() == 0) // Nothing to attack.
	{
		return; // Skip this surge.
	}

	AThreatSpawnerActor* Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)]; // Pick one at random.
	Chosen->SpawnBurst(Config.SurgeThreatCount, EThreatSeverity::Critical); // Send the critical wave.
	PostAlert(FString::Printf(TEXT("ZERO-DAY SURGE: critical exploits inbound on the %s wall!"), WallSideToString(Chosen->TargetWall->WallSide)), FColor(255, 105, 90)); // Warn the player.
}

// Stops the timers and spawners and records the result for the HUD.
void AZeroDayGameMode::EndGame(bool bWon, const FString& Reason)
{
	if (bGameEnded) // Already over?
	{
		return; // Only end once.
	}
	bGameEnded = true; // Latch the ended state.
	bPlayerWon = bWon; // Remember the result.
	EndMessage = Reason; // Remember why, for the end screen.

	LevelTimer->CancelCooldown(); // Stop the level countdown.
	SurgeTimer->CancelCooldown(); // Stop any surges.
	for (AThreatSpawnerActor* Spawner : Spawners) // Stop every spawner.
	{
		if (Spawner) // Skip destroyed ones.
		{
			Spawner->SetSpawningEnabled(false); // No more threats.
		}
	}
}
