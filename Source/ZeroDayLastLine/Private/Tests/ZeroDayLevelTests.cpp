#if WITH_DEV_AUTOMATION_TESTS // Tests only exist in development/editor builds, never in shipping.

#include "Misc/AutomationTest.h" // Unreal's automation test macros.
#include "ZeroDayTestWorld.h" // Helper that creates a headless game world.
#include "ZeroDayLevelConfig.h" // The level rules under test.
#include "WallActor.h" // Walls the tests inspect.
#include "CityTaskActor.h" // Stations the tests inspect.
#include "ThreatSpawnerActor.h" // Spawners the tests inspect.

// Checks the three levels really do get harder and introduce new mechanics.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayLevelConfigTest, "ZeroDay.Levels.Config", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayLevelConfigTest::RunTest(const FString& Parameters)
{
	const FZeroDayLevelConfig L1 = FZeroDayLevelConfig::ForLevel(1); // Level 1 rules.
	const FZeroDayLevelConfig L2 = FZeroDayLevelConfig::ForLevel(2); // Level 2 rules.
	const FZeroDayLevelConfig L3 = FZeroDayLevelConfig::ForLevel(3); // Level 3 rules.

	TestEqual(TEXT("There are exactly three levels"), FZeroDayLevelConfig::MaxLevel, 3); // Rubric needs at least three.
	TestEqual(TEXT("Level numbers match"), L1.Level * 100 + L2.Level * 10 + L3.Level, 123); // 1, 2, 3.
	TestEqual(TEXT("Out of range low clamps to level 1"), FZeroDayLevelConfig::ForLevel(-5).Level, 1); // Clamp low.
	TestEqual(TEXT("Out of range high clamps to level 3"), FZeroDayLevelConfig::ForLevel(99).Level, 3); // Clamp high.

	TestTrue(TEXT("Levels get longer"), L1.DurationSeconds < L2.DurationSeconds && L2.DurationSeconds < L3.DurationSeconds); // Duration grows.
	TestTrue(TEXT("Threats get faster"), L1.ThreatSpeedScale < L2.ThreatSpeedScale && L2.ThreatSpeedScale < L3.ThreatSpeedScale); // Speed grows.
	TestTrue(TEXT("Threats get tougher"), L1.ThreatHealthScale < L2.ThreatHealthScale && L2.ThreatHealthScale < L3.ThreatHealthScale); // Health grows.
	TestTrue(TEXT("More false positives each level"), L1.DecoyChance < L2.DecoyChance && L2.DecoyChance < L3.DecoyChance); // Decoys grow.
	TestTrue(TEXT("More critical threats each level"), L1.CriticalChance < L2.CriticalChance && L2.CriticalChance < L3.CriticalChance); // Criticals grow.
	TestTrue(TEXT("Breach windows shrink"), L1.RemediationWindowSeconds > L2.RemediationWindowSeconds && L2.RemediationWindowSeconds > L3.RemediationWindowSeconds); // Less time to react.
	TestTrue(TEXT("Wall count grows"), L1.ActiveWallCount < L2.ActiveWallCount && L2.ActiveWallCount <= L3.ActiveWallCount); // More walls.
	TestTrue(TEXT("Stations only on level 3"), !L1.bTasksActive && !L2.bTasksActive && L3.bTasksActive); // New mechanic on level 3.
	TestTrue(TEXT("Surges only on level 3"), !L1.bSurgesEnabled && !L2.bSurgesEnabled && L3.bSurgesEnabled); // New mechanic on level 3.
	TestTrue(TEXT("Every level has a title and objective"), !L1.Title.IsEmpty() && !L2.Title.IsEmpty() && !L3.Title.IsEmpty() && !L1.Objective.IsEmpty() && !L2.Objective.IsEmpty() && !L3.Objective.IsEmpty()); // Text for the HUD.
	return true; // The test itself completed; individual checks report their own failures.
}

// Plays the game headlessly with nobody defending and checks the level is lost, then can be restarted.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayLoseAndRestartTest, "ZeroDay.Levels.LoseAndRestart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayLoseAndRestartTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	TestEqual(TEXT("Auto layout built four walls"), Test.GameMode->GetWalls().Num(), 4); // The map was empty so the mode built a city.
	TestEqual(TEXT("Auto layout built four spawners"), Test.GameMode->GetSpawners().Num(), 4); // One spawner per wall.
	TestEqual(TEXT("Auto layout built three stations"), Test.GameMode->GetTasks().Num(), 3); // Power, food, medical.
	TestEqual(TEXT("Starts on level 1"), Test.GameMode->CurrentLevel, 1); // Default level.

	int32 ActiveWalls = 0; // Count walls in play on level 1.
	for (AWallActor* Wall : Test.GameMode->GetWalls()) // Look at each wall.
	{
		ActiveWalls += Wall->bIsActiveThisLevel ? 1 : 0; // Count the active ones.
	}
	TestEqual(TEXT("Level 1 only attacks one wall"), ActiveWalls, 1); // North only.

	// Nobody shoots or fixes anything, so the north wall should fall and the level should be lost.
	float Elapsed = 0.f; // Simulated seconds so far.
	while (!Test.GameMode->IsGameOver() && Elapsed < 150.f) // Level 1 lasts 75 seconds; give it double.
	{
		Test.Tick(1.f); // One simulated second.
		Elapsed += 1.f; // Track time.
	}
	TestTrue(TEXT("Level ends when undefended"), Test.GameMode->IsGameOver()); // Something must end it.
	TestFalse(TEXT("Undefended level is lost"), Test.GameMode->DidPlayerWin()); // A wall fell.
	TestFalse(TEXT("Cannot advance after losing"), Test.GameMode->CanAdvanceLevel()); // Only wins advance.

	Test.GameMode->ReloadLevel(); // Press R.
	TestFalse(TEXT("Restart clears the game over state"), Test.GameMode->IsGameOver()); // Playing again.
	TestEqual(TEXT("Restart clears leftover threats"), Test.CountThreats(), 0); // Fresh start.
	for (AWallActor* Wall : Test.GameMode->GetWalls()) // Every wall.
	{
		TestEqual(TEXT("Restart repairs walls"), Wall->GetHealthPercent(), 1.f); // Back to full health.
	}
	return true; // Done.
}

// Plays all three levels headlessly with a perfect defender and checks each can be won and unlocks the next.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZeroDayFullPlaythroughTest, "ZeroDay.Levels.FullPlaythrough", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZeroDayFullPlaythroughTest::RunTest(const FString& Parameters)
{
	ZeroDayTests::FTestWorld Test; // Build a real game world.
	if (!TestNotNull(TEXT("Game mode was created"), Test.GameMode)) // Nothing works without it.
	{
		return false; // Stop early.
	}

	for (int32 Level = 1; Level <= FZeroDayLevelConfig::MaxLevel; ++Level) // Play levels 1, 2 and 3 in order.
	{
		const FZeroDayLevelConfig Rules = FZeroDayLevelConfig::ForLevel(Level); // What we expect this level to do.
		const FString Label = FString::Printf(TEXT("Level %d"), Level); // For readable failure messages.

		TestEqual(*(Label + TEXT(": game mode is on this level")), Test.GameMode->CurrentLevel, Level); // Advancing worked.
		TestEqual(*(Label + TEXT(": starting Intel")), Test.GameMode->GetResource(EResourceType::Intel), Rules.StartingIntel); // Fresh Intel.
		TestEqual(*(Label + TEXT(": starting Patch kits")), Test.GameMode->GetResource(EResourceType::Patch), Rules.StartingPatch); // Fresh Patch kits.
		TestEqual(*(Label + TEXT(": starting Food")), Test.GameMode->GetResource(EResourceType::Food), Rules.StartingFood); // Fresh Food.
		TestEqual(*(Label + TEXT(": starting survivors")), Test.GameMode->GetSurvivors(), Rules.StartingSurvivors); // Everyone alive.

		int32 ActiveWalls = 0; // Walls in play.
		for (AWallActor* Wall : Test.GameMode->GetWalls()) // Check each wall.
		{
			ActiveWalls += Wall->bIsActiveThisLevel ? 1 : 0; // Count the active ones.
			TestEqual(*(Label + TEXT(": walls start at full health")), Wall->GetHealthPercent(), 1.f); // Reset worked.
		}
		TestEqual(*(Label + TEXT(": active walls")), ActiveWalls, Rules.ActiveWallCount); // 1 on level 1, 4 after.

		int32 ActiveStations = 0; // Stations in play.
		for (ACityTaskActor* Task : Test.GameMode->GetTasks()) // Check each station.
		{
			ActiveStations += Task->bIsActiveThisLevel ? 1 : 0; // Count the active ones.
		}
		TestEqual(*(Label + TEXT(": active stations")), ActiveStations, Rules.bTasksActive ? 3 : 0); // Only level 3.

		// Play the level with a perfect defender: every threat is removed the moment it appears.
		int32 ThreatsSeen = 0; // How many threats appeared.
		int32 DecoysSeen = 0; // How many were false positives.
		int32 CriticalsSeen = 0; // How many were critical.
		bool bSurgeAnnounced = false; // Did a surge alert appear?
		float Elapsed = 0.f; // Simulated seconds so far.
		while (!Test.GameMode->IsGameOver() && Elapsed < Rules.DurationSeconds + 20.f) // Give a little slack past the level length.
		{
			Test.Tick(0.25f, 0.05f); // Advance a quarter second.
			Elapsed += 0.25f; // Track time.

			TArray<AThreatPawn*> Threats; // Everything alive right now.
			for (TActorIterator<AThreatPawn> It(Test.World); It; ++It) // Collect the threats.
			{
				Threats.Add(*It); // Remember it.
			}
			for (AThreatPawn* Threat : Threats) // Record and remove each one.
			{
				++ThreatsSeen; // Count it.
				DecoysSeen += Threat->bIsDecoy ? 1 : 0; // Count false positives.
				CriticalsSeen += Threat->Severity == EThreatSeverity::Critical ? 1 : 0; // Count criticals.
				Threat->ApplyHitDamage(1000000.f); // A perfect shot.
			}

			for (const FZeroDayAlert& Alert : Test.GameMode->GetAlerts()) // Look for the surge warning.
			{
				bSurgeAnnounced |= Alert.Message.Contains(TEXT("SURGE")); // Latch if seen.
			}

			for (ACityTaskActor* Task : Test.GameMode->GetTasks()) // A diligent player also looks after the stations.
			{
				if (Task->bIsActiveThisLevel && Task->GetCondition() < 45.f) // Attend when it is getting low.
				{
					Task->Attend(); // Spend a resource and restore it.
				}
			}
		}

		TestTrue(*(Label + TEXT(": ended")), Test.GameMode->IsGameOver()); // The timer must end it.
		TestTrue(*(Label + TEXT(": was won by a perfect defender")), Test.GameMode->DidPlayerWin()); // Winnable.
		TestTrue(*(Label + TEXT(": threats actually spawned")), ThreatsSeen > 3); // The spawners work.
		AddInfo(FString::Printf(TEXT("%s: %d threats, %d decoys, %d critical, surge announced=%d"), *Label, ThreatsSeen, DecoysSeen, CriticalsSeen, bSurgeAnnounced ? 1 : 0)); // Report the numbers.

		if (Level == 1) // Level 1 rules.
		{
			TestEqual(TEXT("Level 1 has no false positives"), DecoysSeen, 0); // Introductory level.
			TestEqual(TEXT("Level 1 has no critical threats"), CriticalsSeen, 0); // Introductory level.
		}
		if (Level == 2) // Level 2 rules.
		{
			TestTrue(TEXT("Level 2 introduces false positives"), DecoysSeen > 0); // New mechanic.
			TestTrue(TEXT("Level 2 introduces critical threats"), CriticalsSeen > 0); // New mechanic.
			TestFalse(TEXT("Level 2 has no surges"), bSurgeAnnounced); // Not until level 3.
		}
		if (Level == 3) // Level 3 rules.
		{
			TestTrue(TEXT("Level 3 has surges"), bSurgeAnnounced); // New mechanic.
			TestTrue(TEXT("Level 3 surge sent extra critical threats"), CriticalsSeen >= Rules.SurgeThreatCount); // Surge threats are critical.
		}

		if (Level < FZeroDayLevelConfig::MaxLevel) // Not the last level.
		{
			TestTrue(*(Label + TEXT(": can advance after winning")), Test.GameMode->CanAdvanceLevel()); // Next level unlocked.
			Test.GameMode->AdvanceToNextLevel(); // Press N.
		}
		else
		{
			TestTrue(TEXT("Final level marks the game complete"), Test.GameMode->IsFinalLevelComplete()); // Whole game won.
			TestFalse(TEXT("Cannot advance past the final level"), Test.GameMode->CanAdvanceLevel()); // Nothing after level 3.
		}
	}
	return true; // Done.
}

#endif // WITH_DEV_AUTOMATION_TESTS
