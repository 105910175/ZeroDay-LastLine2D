#pragma once // Only include this header once per compilation unit.

#if WITH_DEV_AUTOMATION_TESTS // Tests only exist in development/editor builds, never in shipping.

#include "CoreMinimal.h" // Basic Unreal types.
#include "CoreGlobals.h" // GFrameCounter.
#include "UObject/UObjectGlobals.h" // CollectGarbage.
#include "Engine/World.h" // UWorld.
#include "Engine/Engine.h" // GEngine and world contexts.
#include "Engine/GameInstance.h" // A world needs a game instance to create its game mode.
#include "GameFramework/WorldSettings.h" // Lets us choose the game mode for the test world.
#include "EngineUtils.h" // TActorIterator.
#include "ZeroDayGameMode.h" // The game mode under test.
#include "ThreatPawn.h" // Threats the tests look at and remove.

namespace ZeroDayTests // Keep the helpers out of the global namespace.
{
	// Creates a real (headless) game world running AZeroDayGameMode, and tears it down afterwards.
	struct FTestWorld
	{
		UGameInstance* GameInstance = nullptr; // Owns the world, like in a real game.
		UWorld* World = nullptr; // The temporary world.
		AZeroDayGameMode* GameMode = nullptr; // The game mode running inside it.

		FTestWorld()
		{
			GameInstance = NewObject<UGameInstance>(GEngine); // Create a game instance.
			GameInstance->AddToRoot(); // Keep it safe from garbage collection while the test runs.
			GameInstance->InitializeStandalone(); // Makes a world and registers its world context.
			World = GameInstance->GetWorld(); // The world the instance created.
			World->GetWorldSettings()->DefaultGameMode = AZeroDayGameMode::StaticClass(); // Use our game mode.
			FURL URL; // An empty travel URL (we are not loading a map).
			World->SetGameMode(URL); // Spawn the game mode.
			World->InitializeActorsForPlay(URL); // Prepare actors.
			World->BeginPlay(); // Start the game (this builds the city and starts level 1).
			GameMode = Cast<AZeroDayGameMode>(World->GetAuthGameMode()); // Keep a typed pointer to it.
		}

		~FTestWorld()
		{
			for (TActorIterator<AActor> It(World); It; ++It) // End play on every actor first so timers and delegates are released cleanly.
			{
				It->RouteEndPlay(EEndPlayReason::LevelTransition); // Same call the engine makes when leaving a level.
			}
			GEngine->DestroyWorldContext(World); // Unregister the world from the engine.
			World->DestroyWorld(false); // Destroy it.
			GameInstance->Shutdown(); // Shut the game instance down.
			GameInstance->RemoveFromRoot(); // Let it be garbage collected.
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS); // Purge the dead world now so the next test starts clean.
		}

		// Advances the game by the given number of simulated seconds.
		void Tick(float Seconds, float Step = 0.05f)
		{
			for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += Step) // One frame at a time.
			{
				++GFrameCounter; // Unreal only ticks each tick function once per frame counter value, so advance it manually.
				World->Tick(LEVELTICK_All, Step); // Run a full engine frame.
			}
		}

		// Number of threats currently alive.
		int32 CountThreats() const
		{
			int32 Count = 0; // Running total.
			for (TActorIterator<AThreatPawn> It(World); It; ++It) // Every threat in the world.
			{
				++Count; // Count it.
			}
			return Count; // How many are alive.
		}
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
