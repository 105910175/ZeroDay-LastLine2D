#include "ZeroDayLevelConfig.h" // The struct this file fills in.

FZeroDayLevelConfig FZeroDayLevelConfig::ForLevel(int32 Level)
{
	FZeroDayLevelConfig Config; // Start from the defaults, then override per level below.
	Config.Level = FMath::Clamp(Level, 1, MaxLevel); // Never go outside the three real levels.

	switch (Config.Level) // Pick the rules for this level.
	{
	case 1: // Level 1 teaches Detect: one wall, slow weak threats, everything is real.
		Config.Title = TEXT("Level 1: Detect"); // Name for the HUD.
		Config.Objective = TEXT("Hold the north wall. Scan (Q) to spot threats, then shoot them before they reach the wall."); // Player goal.
		Config.DurationSeconds = 75.f; // Shortest level.
		Config.ActiveWallCount = 1; // Only the north wall is attacked.
		Config.bTasksActive = false; // No city stations yet.
		Config.SpawnInterval = 5.f; // A threat every five seconds.
		Config.MaxConcurrentThreats = 3; // Never more than three at once.
		Config.ThreatSpeedScale = 0.8f; // Slower than normal.
		Config.ThreatHealthScale = 0.8f; // Weaker than normal.
		Config.DecoyChance = 0.f; // No false positives yet.
		Config.CriticalChance = 0.f; // No critical threats yet.
		Config.MediumChance = 0.25f; // Mostly low severity.
		Config.RemediationWindowSeconds = 15.f; // Generous time to fix a breach.
		Config.bSurgesEnabled = false; // No surge waves.
		Config.StartingResources = 6; // Enough to try Validate and Fix.
		break; // Done with level 1.

	case 2: // Level 2 teaches Validate: every wall, and some threats are false positives.
		Config.Title = TEXT("Level 2: Validate"); // Name for the HUD.
		Config.Objective = TEXT("Every wall is under attack and some threats are false positives. Scan, validate, then stop the real ones first."); // Player goal.
		Config.DurationSeconds = 100.f; // Longer than level 1.
		Config.ActiveWallCount = 4; // All four walls are attacked.
		Config.bTasksActive = false; // Stations still off.
		Config.SpawnInterval = 6.f; // Per wall, but there are four walls now.
		Config.MaxConcurrentThreats = 3; // Three per wall.
		Config.ThreatSpeedScale = 1.f; // Normal speed.
		Config.ThreatHealthScale = 1.f; // Normal health.
		Config.DecoyChance = 0.30f; // Nearly a third are harmless decoys.
		Config.CriticalChance = 0.15f; // Some critical threats.
		Config.MediumChance = 0.35f; // A fair number of medium ones.
		Config.RemediationWindowSeconds = 12.f; // Less time to fix breaches.
		Config.bSurgesEnabled = false; // Still no surges.
		Config.StartingResources = 8; // A few more resources for the extra walls.
		break; // Done with level 2.

	default: // Level 3 teaches Remediate under pressure: stations, surges and the hardest threats.
		Config.Title = TEXT("Level 3: Remediate"); // Name for the HUD.
		Config.Objective = TEXT("Zero-day surges hit every wall while the power, food and medical stations run down. Triage: patch, contain, and keep the survivors alive."); // Player goal.
		Config.DurationSeconds = 120.f; // Longest level.
		Config.ActiveWallCount = 4; // All four walls.
		Config.bTasksActive = true; // City stations now decay and need attention.
		Config.SpawnInterval = 5.f; // Faster spawning.
		Config.MaxConcurrentThreats = 4; // Four per wall.
		Config.ThreatSpeedScale = 1.2f; // Faster than normal.
		Config.ThreatHealthScale = 1.3f; // Tougher than normal.
		Config.DecoyChance = 0.35f; // Many false positives to sort through.
		Config.CriticalChance = 0.25f; // A quarter are critical.
		Config.MediumChance = 0.40f; // Plenty of medium threats.
		Config.RemediationWindowSeconds = 10.f; // Tight time to fix breaches.
		Config.TaskDecayScale = 1.f; // Stations run down at their normal rate.
		Config.bSurgesEnabled = true; // Periodic zero-day surge waves.
		Config.SurgeIntervalSeconds = 40.f; // A surge every forty seconds.
		Config.SurgeThreatCount = 4; // Four critical threats per surge.
		Config.StartingResources = 10; // The most starting resources, and the most demands on them.
		break; // Done with level 3.
	}

	return Config; // Hand the finished rules back to the caller.
}
