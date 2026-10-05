#if WITH_DEV_AUTOMATION_TESTS // Tests only exist in development/editor builds, never in shipping.

#include "Misc/AutomationTest.h" // Unreal's automation test macros.
#include "ZeroDayTestWorld.h" // Helper that creates a headless game world.
#include "WallActor.h" // Walls whose size is checked.
#include "CityTaskActor.h" // Stations whose size is checked.
#include "ThreatSpawnerActor.h" // Spawners used to create threats.
#include "ThreatPawn.h" // Threats whose size is checked.
#include "ZeroDayCharacter.h" // The player whose speed is checked.
#include "Components/StaticMeshComponent.h" // The wall and station bodies.
#include "Components/SphereComponent.h" // The threat's collision body.
#include "PaperSpriteComponent.h" // The threat and station artwork.
#include "GameFramework/CharacterMovementComponent.h" // Walking speed.

// Threats are drawn and collide at 4x their original size.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayThreatSizeTest, "ZeroDay.Sizes.Threats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayThreatSizeTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	AThreatSpawnerActor* Spawner = Test.GameMode->GetSpawners().Num() > 0 ? Test.GameMode->GetSpawners()[0].Get() : nullptr; // Any spawner will do.
	if (!TestNotNull(TEXT("A spawner exists"), Spawner)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	TestTrue(TEXT("A threat can be spawned"), Spawner->SpawnThreat(EThreatSeverity::Medium, false)); // Create one.
	AThreatPawn* Threat = nullptr; // The threat we just made.
	for (TActorIterator<AThreatPawn> It(Test.World); It; ++It) // Find it.
	{
		Threat = *It; // Remember it.
	}
	if (!TestNotNull(TEXT("The threat exists"), Threat)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	TestNearlyEqual(TEXT("The threat's scale setting is 4x"), Threat->VisualScale, 4.f); // The requested size.
	TestNearlyEqual(TEXT("The collision body grew from 25 to 100 units"), Threat->GetBodyRadius(), 100.f); // 25 x 4, the same size as the drawn threat.
	const UPaperSpriteComponent* Sprite = Threat->FindComponentByClass<UPaperSpriteComponent>(); // Its artwork.
	if (TestNotNull(TEXT("The threat has artwork"), Sprite)) // Only check it if present.
	{
		TestNearlyEqual(TEXT("The artwork is drawn 4x bigger"), static_cast<float>(Sprite->GetRelativeScale3D().X), 4.f); // Matches the collision.
	}
	return true; // Done.
}

// Walls are twice as thick and tall as before, and their corners still join.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayWallSizeTest, "ZeroDay.Sizes.Walls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayWallSizeTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	TestEqual(TEXT("Four walls were built"), Test.GameMode->GetWalls().Num(), 4); // North, South, East, West.
	for (AWallActor* Wall : Test.GameMode->GetWalls()) // Check each one.
	{
		if (!TestNotNull(TEXT("The wall exists"), Wall)) // Needed for the rest.
		{
			continue; // Next wall.
		}
		TestNearlyEqual(TEXT("The wall's scale setting is 2x"), Wall->WallSizeScale, 2.f); // The requested size.

		const UStaticMeshComponent* Body = Wall->FindComponentByClass<UStaticMeshComponent>(); // The solid body.
		if (TestNotNull(TEXT("The wall has a body"), Body)) // Only check it if present.
		{
			TestNearlyEqual(TEXT("Body thickness doubled (96 -> 192 units)"), static_cast<float>(Body->GetRelativeScale3D().Y * 100.f), 192.f); // Depth axis.
			TestNearlyEqual(TEXT("Body height doubled (60 -> 120 units)"), static_cast<float>(Body->GetRelativeScale3D().Z * 100.f), 120.f); // Height axis.
		}
	}

	const float OverlapNeeded = 96.f * 2.f * 0.5f; // Half the new wall thickness: the least the corners must overlap by to stay joined.
	TestTrue(TEXT("North/south walls still overlap the corners by at least half a wall thickness"), Test.GameMode->WallCornerOverlap >= OverlapNeeded); // No corner gaps.
	return true; // Done.
}

// Stations are three times their original size, and are reached from their edge, not just their centre.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayStationSizeTest, "ZeroDay.Sizes.Stations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayStationSizeTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	TestEqual(TEXT("Three stations were built"), Test.GameMode->GetTasks().Num(), 3); // Power, Food, Medical.
	for (ACityTaskActor* Task : Test.GameMode->GetTasks()) // Check each one.
	{
		if (!TestNotNull(TEXT("The station exists"), Task)) // Needed for the rest.
		{
			continue; // Next station.
		}
		TestNearlyEqual(TEXT("The station's scale setting is 3x"), Task->StationScale, 3.f); // The requested size.
		TestNearlyEqual(TEXT("The station can be used from 195 units from its centre"), Task->GetInteractRadius(), 195.f); // 65 x 3.

		const UStaticMeshComponent* Body = Task->FindComponentByClass<UStaticMeshComponent>(); // The solid body.
		if (TestNotNull(TEXT("The station has a body"), Body)) // Only check it if present.
		{
			TestNearlyEqual(TEXT("Body width tripled (130 -> 390 units)"), static_cast<float>(Body->GetRelativeScale3D().X * 100.f), 390.f); // Width axis.
		}
		const UPaperSpriteComponent* Sprite = Task->FindComponentByClass<UPaperSpriteComponent>(); // Its artwork.
		if (TestNotNull(TEXT("The station has artwork"), Sprite)) // Only check it if present.
		{
			TestNearlyEqual(TEXT("Artwork tripled (0.85 -> 2.55)"), static_cast<float>(Sprite->GetRelativeScale3D().X), 2.55f); // Matches the body.
		}
	}
	return true; // Done.
}

// The player walks twice as fast as the character's original speed.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayWalkSpeedTest, "ZeroDay.Movement.WalkSpeed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayWalkSpeedTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	const float OriginalSpeed = GetDefault<AZeroDayCharacter>()->GetCharacterMovement()->MaxWalkSpeed; // What the class starts with.
	AZeroDayCharacter* Player = Test.World->SpawnActor<AZeroDayCharacter>(FVector::ZeroVector, FRotator::ZeroRotator); // The player (BeginPlay applies the multiplier).
	if (!TestNotNull(TEXT("The player was spawned"), Player)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	TestNearlyEqual(TEXT("The multiplier setting is 2x"), Player->WalkSpeedMultiplier, 2.f); // The requested change.
	TestNearlyEqual(TEXT("Walking speed is doubled"), Player->GetCharacterMovement()->MaxWalkSpeed, OriginalSpeed * 2.f); // 600 -> 1200.
	return true; // Done.
}

// Regression: with bigger threats and walls, a freshly spawned threat must not already be touching its wall
// (that made it vanish the moment it appeared, so spawners seemed to produce nothing).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDaySpawnClearTest, "ZeroDay.Spawning.NewThreatsSurvive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDaySpawnClearTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	Test.GameMode->StartLevel(2); // Level 2 attacks every wall.
	int32 Spawned = 0; // How many threats we asked for.
	for (AThreatSpawnerActor* Spawner : Test.GameMode->GetSpawners()) // One per wall.
	{
		for (int32 Index = 0; Index < 3; ++Index) // Several each, to cover the random scatter.
		{
			if (Spawner && Spawner->SpawnThreat(EThreatSeverity::Medium, false)) // Create a real threat.
			{
				++Spawned; // Count it.
			}
		}
	}
	Test.Tick(0.3f); // A few frames: anything spawned touching its wall would be gone by now.
	TestEqual(TEXT("Every spawned threat is still alive after a moment"), Test.CountThreats(), Spawned); // None vanished.
	return true; // Done.
}

#endif // WITH_DEV_AUTOMATION_TESTS
