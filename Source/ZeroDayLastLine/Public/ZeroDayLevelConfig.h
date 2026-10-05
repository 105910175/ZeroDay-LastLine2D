#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types (int32, float, FString, ...).

// Plain description of one level's rules. Every difficulty knob lives here so the
// three levels can be compared (and tested) side by side without touching gameplay code.
struct ZERODAYLASTLINE_API FZeroDayLevelConfig
{
	// The highest level number in the game. After this one the game is complete.
	static constexpr int32 MaxLevel = 3; // Three levels in total.

	int32 Level = 1;                       // Which level these rules belong to (1-3).
	FString Title;                         // Short name shown on the HUD, e.g. "Level 1: Detect".
	FString Objective;                     // One or two sentences telling the player what to do.
	float DurationSeconds = 75.f;          // How long the player must hold out to win.
	int32 ActiveWallCount = 1;             // 1 = only the north wall is attacked, 4 = every wall.
	bool bTasksActive = false;             // True when the city stations (power/food/medical) run.
	float SpawnInterval = 5.f;             // Seconds between spawns, per spawner.
	int32 MaxConcurrentThreats = 3;        // Live threats allowed per spawner at once.
	float ThreatSpeedScale = 1.f;          // Multiplies how fast threats walk.
	float ThreatHealthScale = 1.f;         // Multiplies how many shots threats take.
	float DecoyChance = 0.f;               // Chance (0-1) a threat is a harmless false positive.
	float CriticalChance = 0.f;            // Chance (0-1) a threat is Critical severity.
	float MediumChance = 0.25f;            // Chance (0-1) a threat is Medium (the rest are Low).
	float RemediationWindowSeconds = 15.f; // Time to fix a breached wall before it is lost.
	float TaskDecayScale = 1.f;            // Multiplies how quickly city stations run down.
	bool bSurgesEnabled = false;           // True when periodic zero-day surge waves happen.
	float SurgeIntervalSeconds = 40.f;     // Seconds between surge waves.
	int32 SurgeThreatCount = 4;            // Critical threats sent in each surge.
	int32 StartingIntel = 3;               // Intel (spent on Validate) at the start of the level.
	int32 StartingPatch = 3;               // Patch kits (spent on fixing walls) at the start.
	int32 StartingFood = 0;                // Food (spent at the food store) at the start.
	int32 StartingPower = 0;               // Power cells (spent at the generator) at the start.
	int32 StartingMedical = 0;             // Medical kits (spent at the medical tent) at the start.
	int32 StartingSurvivors = 5;           // Survivors sheltering in the city. Failing stations costs lives.
	float SupplyIntervalSeconds = 14.f;    // Seconds between supply drops inside the city.
	float ThreatDropChance = 0.4f;         // Chance (0-1) a real threat drops a resource when shot.

	// Returns the rules for a level. Out-of-range levels are clamped into 1..MaxLevel.
	static FZeroDayLevelConfig ForLevel(int32 Level);
};
