#include "ZeroDayDevDemo.h" // This class's own declaration.
#include "ZeroDayGameMode.h" // The game being driven.
#include "WallActor.h" // Walls to damage.
#include "ThreatSpawnerActor.h" // Spawners used to create threats on demand.
#include "DetectionComponent.h" // The player's scan and validate tools.
#include "Engine/World.h" // UWorld and timers.
#include "UnrealClient.h" // FScreenshotRequest.
#include "Misc/CommandLine.h" // Reads the launch arguments.
#include "Misc/Parse.h" // FParse::Param.
#include "TimerManager.h" // FTimerManager.
#include "Kismet/GameplayStatics.h" // GetPlayerPawn.
#include "ThreatPawn.h" // So the script can slow threats down.
#include "EngineUtils.h" // TActorIterator.
#include "Camera/CameraComponent.h" // Camera diagnostics.
#include "GameFramework/SpringArmComponent.h" // Camera boom diagnostics.

// The demo only exists when explicitly requested, and never in shipping builds.
bool UZeroDayDevDemo::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false; // Never in shipping.
#else
	UWorld* World = Cast<UWorld>(Outer); // The world this would live in.
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("ZeroDayDemo")); // Game worlds only, and only when asked for.
#endif
}

// Start stepping through the script once gameplay has begun.
void UZeroDayDevDemo::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld); // Let the base class do its setup.
	InWorld.GetTimerManager().SetTimer(StepTimer, this, &UZeroDayDevDemo::RunNextStep, 1.5f, true, 3.f); // One step every 1.5 seconds, after a 3 second warm-up.
}

// Each step either changes the game state or takes a screenshot of it.
void UZeroDayDevDemo::RunNextStep()
{
	UWorld* World = GetWorld(); // The world we are driving.
	AZeroDayGameMode* GameMode = World ? World->GetAuthGameMode<AZeroDayGameMode>() : nullptr; // Its game mode.
	if (!GameMode) // Nothing to drive.
	{
		return; // Wait for next time.
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0); // The player's pawn.
	UDetectionComponent* Detection = Player ? Player->FindComponentByClass<UDetectionComponent>() : nullptr; // Scan and validate tools.

	// Finds the spawner that sends threats at a given wall, so the script can create threats on demand.
	auto SpawnAt = [&](EWallSide Side, EThreatSeverity Severity, bool bDecoy) // Helper: spawn a threat at a wall.
	{
		for (AThreatSpawnerActor* Spawner : GameMode->GetSpawners()) // Look at each spawner.
		{
			if (Spawner && Spawner->TargetWall && Spawner->TargetWall->WallSide == Side) // The right one?
			{
				Spawner->SpawnThreat(Severity, bDecoy); // Create the threat.
			}
		}
		for (TActorIterator<AThreatPawn> It(World); It; ++It) // Slow every threat right down so it lingers for the screenshots.
		{
			It->MoveSpeed = FMath::Min(It->MoveSpeed, 30.f); // Crawl instead of walk.
		}
	};

	// Saves a screenshot with a readable name under Saved/Screenshots.
	auto Shot = [&](const TCHAR* Name) // Helper: take a screenshot.
	{
		FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("ZeroDayDemo_%s"), Name), true, false); // Ask the engine to save the next frame (HUD included) under Saved/Screenshots.
	};

	switch (StepIndex++) // Run the current step, then move on.
	{
	case 0: // Log how the player and camera are set up, and create the first threats.
		if (Player)
		{
			const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>(); // The player's camera.
			const USpringArmComponent* Boom = Player->FindComponentByClass<USpringArmComponent>(); // Its boom.
			UE_LOG(LogTemp, Display, TEXT("ZDDEMO pawn=%s loc=%s rot=%s boomAbsRot=%d camRot=%s"), *Player->GetClass()->GetName(), *Player->GetActorLocation().ToString(), *Player->GetActorRotation().ToString(), Boom ? Boom->IsUsingAbsoluteRotation() : -1, Camera ? *Camera->GetComponentRotation().ToString() : TEXT("none")); // One diagnostic line.
		}
		SpawnAt(EWallSide::North, EThreatSeverity::Critical, false); SpawnAt(EWallSide::North, EThreatSeverity::Low, true); break; // Level 1: two threats arrive.
	case 1: Shot(TEXT("01_level1_start")); break; // Objective banner and unscanned threats.
	case 2: // Walk the player up to the north wall, so a screenshot can show its actual size next to the player (the whole-city shots never get this close).
		if (Player) // Should always exist by now.
		{
			for (AWallActor* Wall : GameMode->GetWalls()) // Find the north wall.
			{
				if (Wall && Wall->WallSide == EWallSide::North) // The one to stand next to.
				{
					// Approach along the direction from the player's own spawn point to the wall, rather than
					// assuming a fixed world axis: the city's "forward" direction depends on the character's
					// ForwardInputDirection (world +X in this game), not a fixed +Y.
					const FVector ToWall = (Wall->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D(); // Direction from the player to the wall.
					Player->SetActorLocation(Wall->GetActorLocation() - ToWall * 220.f); // Just inside the wall, close enough to read its art clearly.
					break; // Done.
				}
			}
		}
		break;
	case 3: Shot(TEXT("01b_wall_closeup")); break; // The wall's actual size and design next to the player.
	case 4: // Bring the threats into view just outside the north wall (they normally start well off screen), and stay by the wall so Validate is in range.
		for (AWallActor* Wall : GameMode->GetWalls()) // Find the north wall.
		{
			if (Wall && Wall->WallSide == EWallSide::North && Player) // The one the threats are heading for.
			{
				const FVector OutDirection = (Wall->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D(); // Away from the player, through the wall.
				int32 Slot = 0; // Spreads the threats out sideways.
				for (TActorIterator<AThreatPawn> It(World); It; ++It) // Every threat.
				{
					const FVector Sideways = FVector(-OutDirection.Y, OutDirection.X, 0.f); // Along the wall.
					It->SetActorLocation(Wall->GetActorLocation() + OutDirection * 700.f + Sideways * (Slot - 1) * 450.f); // Just beyond the wall, in a row.
					++Slot; // Next position.
				}
			}
		}
		break;
	case 5: if (Detection) { Detection->PerformScan(); } break; // Insta Scan.
	case 6: Shot(TEXT("02_level1_scanned")); break; // Scan ring and "?" markers.
	case 7: if (Detection) { Detection->PerformValidate(); } break; // Validate.
	case 8: Shot(TEXT("03_level1_validated")); break; // Severity and FALSE POSITIVE labels.
	case 9: if (Player) { Player->SetActorLocation(FVector::ZeroVector); Player->SetActorRotation(FRotator(0.f, 90.f, 0.f)); } break; // Back to the city centre, and turn the character, as walking would.
	case 10: // Did the camera turn with it?
		if (Player)
		{
			const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>(); // The player's camera.
			UE_LOG(LogTemp, Display, TEXT("ZDDEMO after turning: rot=%s camRot=%s"), *Player->GetActorRotation().ToString(), Camera ? *Camera->GetComponentRotation().ToString() : TEXT("none")); // Log both.
		}
		Shot(TEXT("04_after_turn")); break; // The view after the turn.
	case 11: if (Player) { Player->SetActorRotation(FRotator::ZeroRotator); } if (UFunction* Win = GameMode->FindFunction(TEXT("HandleLevelSurvived"))) { GameMode->ProcessEvent(Win, nullptr); } break; // Turn back, then end level 1 as a win.
	case 12: Shot(TEXT("05_level1_complete")); break; // Level complete screen offering level 2.
	case 13: GameMode->AdvanceToNextLevel(); break; // Press N.
	case 14: SpawnAt(EWallSide::East, EThreatSeverity::Medium, false); SpawnAt(EWallSide::West, EThreatSeverity::Critical, false); SpawnAt(EWallSide::South, EThreatSeverity::Low, true); break; // Threats from several sides.
	case 15: Shot(TEXT("06_level2_start")); break; // Level 2: all four walls.
	case 16: GameMode->StartLevel(3); break; // Jump to level 3.
	case 17: Shot(TEXT("07_level3_hud")); break; // Resources, survivors, stations, walls.
	case 18: for (AWallActor* Wall : GameMode->GetWalls()) { if (Wall && Wall->WallSide == EWallSide::North) { Wall->TakeThreatDamage(1000.f); } } break; // Breach the north wall.
	case 19: Shot(TEXT("08_level3_breach")); break; // Breach alert, countdown and red ring.
	case 20: if (UFunction* Win = GameMode->FindFunction(TEXT("HandleLevelSurvived"))) { GameMode->ProcessEvent(Win, nullptr); } break; // Finish the last level (a win).
	case 21: Shot(TEXT("09_all_levels_cleared")); break; // Final result screen.
	default: FPlatformMisc::RequestExit(false); break; // Finished: quit the game.
	}
}
