#if WITH_DEV_AUTOMATION_TESTS // Tests only exist in development/editor builds, never in shipping.

#include "Misc/AutomationTest.h" // Unreal's automation test macros.
#include "ZeroDayTestWorld.h" // Helper that creates a headless game world.
#include "WallActor.h" // Walls the tests break and fix.
#include "CityTaskActor.h" // Stations the tests attend.
#include "ThreatSpawnerActor.h" // Spawners used to create specific threats.
#include "ZeroDayCharacter.h" // The player, who owns the scan and validate tools.
#include "DetectionComponent.h" // Scan and validate.

namespace // Helpers only this file uses.
{
	// Finds the wall on a given side of the city.
	AWallActor* FindWall(AZeroDayGameMode* GameMode, EWallSide Side)
	{
		for (AWallActor* Wall : GameMode->GetWalls()) // Look at every wall.
		{
			if (Wall && Wall->WallSide == Side) // Is this the one?
			{
				return Wall; // Found it.
			}
		}
		return nullptr; // No wall on that side.
	}

	// Finds the station of a given kind.
	ACityTaskActor* FindStation(AZeroDayGameMode* GameMode, ECityTaskKind Kind)
	{
		for (ACityTaskActor* Task : GameMode->GetTasks()) // Look at every station.
		{
			if (Task && Task->TaskKind == Kind) // Is this the one?
			{
				return Task; // Found it.
			}
		}
		return nullptr; // No such station.
	}

	// Removes every threat so walls are not damaged while a test does something else.
	void KillAllThreats(ZeroDayTests::FTestWorld& Test)
	{
		TArray<AThreatPawn*> Threats; // Everything alive.
		for (TActorIterator<AThreatPawn> It(Test.World); It; ++It) // Collect the threats first.
		{
			Threats.Add(*It); // Remember it.
		}
		for (AThreatPawn* Threat : Threats) // Now remove them.
		{
			Threat->ApplyHitDamage(1000000.f); // A perfect shot.
		}
	}
}

// Checks the resource inventory: adding, spending, affordability and all-or-nothing spending.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayResourceTest, "ZeroDay.Economy.Resources", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayResourceTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}
	AZeroDayGameMode* GM = Test.GameMode; // Shorter name.

	TestEqual(TEXT("Level 1 starts with 3 Intel"), GM->GetResource(EResourceType::Intel), 3); // From the level table.
	TestEqual(TEXT("Level 1 starts with no Food"), GM->GetResource(EResourceType::Food), 0); // Stations are off on level 1.
	TestTrue(TEXT("Can afford 3 Intel"), GM->CanAfford(EResourceType::Intel, 3)); // Exactly enough.
	TestFalse(TEXT("Cannot afford 4 Intel"), GM->CanAfford(EResourceType::Intel, 4)); // One too many.

	TestFalse(TEXT("Overspending fails"), GM->SpendResource(EResourceType::Intel, 4)); // Cannot pay.
	TestEqual(TEXT("A failed spend takes nothing"), GM->GetResource(EResourceType::Intel), 3); // All-or-nothing.
	TestTrue(TEXT("Spending within budget works"), GM->SpendResource(EResourceType::Intel, 2)); // Pay two.
	TestEqual(TEXT("Spending reduces the count"), GM->GetResource(EResourceType::Intel), 1); // One left.

	GM->AddResource(EResourceType::Food, 5); // Pick up some food.
	TestEqual(TEXT("Adding increases the count"), GM->GetResource(EResourceType::Food), 5); // Five now.
	GM->AddResource(EResourceType::Food, -99); // Absurd negative amount.
	TestEqual(TEXT("Counts never go below zero"), GM->GetResource(EResourceType::Food), 0); // Clamped.

	TestEqual(TEXT("Power station uses Power"), static_cast<int32>(ResourceForTask(ECityTaskKind::Power)), static_cast<int32>(EResourceType::Power)); // Mapping.
	TestEqual(TEXT("Food station uses Food"), static_cast<int32>(ResourceForTask(ECityTaskKind::Food)), static_cast<int32>(EResourceType::Food)); // Mapping.
	TestEqual(TEXT("Medic station uses Medical"), static_cast<int32>(ResourceForTask(ECityTaskKind::Medic)), static_cast<int32>(EResourceType::Medical)); // Mapping.
	return true; // Done.
}

// Checks that fixing a wall costs a Patch kit, needs a real breach, and is counted.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayFixWallTest, "ZeroDay.Economy.FixWall", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayFixWallTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}
	AZeroDayGameMode* GM = Test.GameMode; // Shorter name.
	AWallActor* North = FindWall(GM, EWallSide::North); // The wall in play on level 1.
	if (!TestNotNull(TEXT("North wall exists"), North)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	TestFalse(TEXT("An intact wall cannot be fixed"), North->AttemptFix()); // Nothing to fix.
	TestEqual(TEXT("Nothing was spent"), GM->GetResource(EResourceType::Patch), 3); // Still 3 Patch kits.

	North->TakeThreatDamage(1000.f); // Breach it.
	TestTrue(TEXT("Wall is breached"), North->GetStatus() == EWallStatus::Breached); // Health gone.

	GM->SpendResource(EResourceType::Patch, 3); // Empty the Patch kits.
	TestFalse(TEXT("Cannot fix without a Patch kit"), North->AttemptFix()); // Cannot pay.
	TestTrue(TEXT("Wall is still breached"), North->GetStatus() == EWallStatus::Breached); // Unchanged.

	GM->AddResource(EResourceType::Patch, 1); // Collect one.
	TestTrue(TEXT("Fixing with a Patch kit works"), North->AttemptFix()); // Now it works.
	TestTrue(TEXT("Wall is repaired"), North->GetStatus() == EWallStatus::Safe); // Back to full.
	TestEqual(TEXT("The Patch kit was spent"), GM->GetResource(EResourceType::Patch), 0); // Used up.
	TestEqual(TEXT("The fix was counted"), GM->GetStats().WallsFixed, 1); // For the result screen.
	return true; // Done.
}

// Checks each station spends its own resource, and that failing stations cost survivors then the level.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDaySurvivorsTest, "ZeroDay.Economy.Survivors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDaySurvivorsTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}
	AZeroDayGameMode* GM = Test.GameMode; // Shorter name.
	GM->StartLevel(3); // Stations only run on level 3.

	ACityTaskActor* Food = FindStation(GM, ECityTaskKind::Food); // The food store.
	ACityTaskActor* Power = FindStation(GM, ECityTaskKind::Power); // The generator.
	ACityTaskActor* Medic = FindStation(GM, ECityTaskKind::Medic); // The medical tent.
	if (!TestTrue(TEXT("All three stations exist"), Food && Power && Medic)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	Test.Tick(10.f); // Let the meters drop a little.
	const float FoodBefore = Food->GetCondition(); // Food store condition.
	Food->Attend(); // Feed the survivors.
	TestTrue(TEXT("Attending the food store restores it"), Food->GetCondition() > FoodBefore); // Went back up.
	TestEqual(TEXT("Attending spends Food"), GM->GetResource(EResourceType::Food), 2); // 3 -> 2.
	TestEqual(TEXT("Attending does not spend Power"), GM->GetResource(EResourceType::Power), 3); // Untouched.
	Power->Attend(); // Restart the generator.
	TestEqual(TEXT("Attending the generator spends Power"), GM->GetResource(EResourceType::Power), 2); // 3 -> 2.
	Medic->Attend(); // Treat survivors.
	TestEqual(TEXT("Attending the medical tent spends Medical"), GM->GetResource(EResourceType::Medical), 2); // 3 -> 2.

	GM->SpendResource(EResourceType::Food, GM->GetResource(EResourceType::Food)); // Run out of Food.
	Food->Attend(); // Try to attend with nothing.
	TestEqual(TEXT("Cannot attend without the resource"), GM->GetResource(EResourceType::Food), 0); // Still zero.

	// Leave the stations alone while keeping every wall safe. Survivors should die off one at a time.
	int32 LowestSurvivors = GM->GetSurvivors(); // Track how far it drops.
	float Elapsed = 0.f; // Simulated seconds.
	while (!GM->IsGameOver() && Elapsed < 200.f) // Stations fail every ~20-45 seconds.
	{
		KillAllThreats(Test); // Walls never fall, so only the stations can end the level.
		Test.Tick(0.25f); // Advance a little.
		Elapsed += 0.25f; // Track time.
		LowestSurvivors = FMath::Min(LowestSurvivors, GM->GetSurvivors()); // Record the lowest.
	}
	TestTrue(TEXT("Survivors were lost to failing stations"), GM->GetStats().SurvivorsLost > 0); // The mechanic fires.
	TestEqual(TEXT("All survivors were lost"), LowestSurvivors, 0); // Every one died.
	TestTrue(TEXT("Level ended"), GM->IsGameOver()); // Losing the last survivor ends it.
	TestFalse(TEXT("Level was lost"), GM->DidPlayerWin()); // A loss.
	return true; // Done.
}

// Checks Insta Scan then Validate: scan is free, Validate costs Intel, reveals threats, and is counted.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayValidateTest, "ZeroDay.Economy.ScanAndValidate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayValidateTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}
	AZeroDayGameMode* GM = Test.GameMode; // Shorter name.
	AWallActor* North = FindWall(GM, EWallSide::North); // The wall in play on level 1.
	AThreatSpawnerActor* Spawner = nullptr; // The spawner that sends threats at it.
	for (AThreatSpawnerActor* Candidate : GM->GetSpawners()) // Find the north spawner.
	{
		if (Candidate && Candidate->TargetWall == North) // Is it the right one?
		{
			Spawner = Candidate; // Found it.
		}
	}
	if (!TestTrue(TEXT("North wall and spawner exist"), North && Spawner)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	// Standing close to the north wall, as if having walked up to defend it: the city is now
	// large enough that Validate's range no longer reaches every wall from dead centre, which
	// is the point (defending a wall means being at it, not sitting in the middle of the city).
	const FVector PlayerSpawnLocation = North->GetActorLocation() - FVector(0.f, 300.f, 0.f); // Just inside the north wall.
	AZeroDayCharacter* Player = Test.World->SpawnActor<AZeroDayCharacter>(PlayerSpawnLocation, FRotator::ZeroRotator); // Spawn the player there.
	UDetectionComponent* Detection = Player ? Player->FindComponentByClass<UDetectionComponent>() : nullptr; // Their scan and validate tools.
	if (!TestNotNull(TEXT("The player has detection tools"), Detection)) // Needed for the rest.
	{
		return false; // Stop early.
	}

	// Create one real critical threat and one false positive heading for the north wall.
	Spawner->SpawnThreat(EThreatSeverity::Critical, false); // A real, critical one.
	Spawner->SpawnThreat(EThreatSeverity::Low, true); // A harmless false positive.
	TestEqual(TEXT("Two threats are inbound"), North->GetInboundThreatCount(), 2); // Registered with the wall.
	TestEqual(TEXT("Nothing is detected before scanning"), North->GetDetectedThreatCount(), 0); // Unknown to the player.

	Detection->PerformValidate(); // Try to validate without scanning first.
	TestEqual(TEXT("Validate needs a scan first"), GM->GetResource(EResourceType::Intel), 3); // Nothing spent.

	Detection->PerformScan(); // Insta Scan (free).
	TestEqual(TEXT("Scan flagged both threats"), North->GetDetectedThreatCount(), 2); // Both found.
	TestEqual(TEXT("Scan is free"), GM->GetResource(EResourceType::Intel), 3); // No cost.
	TestTrue(TEXT("Scan starts a cooldown"), Detection->GetScanCooldownRemaining() > 0.f); // Recharging.
	TestEqual(TEXT("Nothing is validated yet"), North->GetUnvalidatedDetectedCount(), 2); // Still unknown.

	Detection->PerformValidate(); // Validate the scanned wall.
	TestEqual(TEXT("Validate costs one Intel"), GM->GetResource(EResourceType::Intel), 2); // 3 -> 2.
	TestEqual(TEXT("Everything is now validated"), North->GetUnvalidatedDetectedCount(), 0); // Revealed.
	TestEqual(TEXT("The false positive was counted"), GM->GetStats().FalsePositivesIdentified, 1); // For the result screen.

	Detection->PerformValidate(); // Validating again changes nothing.
	TestEqual(TEXT("Re-validating is free"), GM->GetResource(EResourceType::Intel), 2); // Nothing more spent.

	GM->SpendResource(EResourceType::Intel, 2); // Run out of Intel.
	Test.Tick(9.f); // Wait for the scan cooldown (8 seconds) to pass.
	TestTrue(TEXT("Scan has recharged"), Detection->GetScanCooldownRemaining() <= 0.f); // Ready again.
	Spawner->SpawnThreat(EThreatSeverity::Medium, false); // A fresh, unknown threat.
	Detection->PerformScan(); // Flag it.
	TestTrue(TEXT("The new threat was flagged"), North->GetUnvalidatedDetectedCount() > 0); // Detected but unknown.
	const int32 UnknownBefore = North->GetUnvalidatedDetectedCount(); // How many are still unknown.
	Detection->PerformValidate(); // Try to validate with no Intel.
	TestEqual(TEXT("Validate is refused without Intel"), North->GetUnvalidatedDetectedCount(), UnknownBefore); // Nothing was revealed.
	return true; // Done.
}

#endif // WITH_DEV_AUTOMATION_TESTS
