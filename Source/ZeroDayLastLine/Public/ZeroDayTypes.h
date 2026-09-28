#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "ZeroDayTypes.generated.h" // Unreal's generated reflection code for the enums below.

// Shared enums and small helpers used across the game's systems.
//
// Collision channels (set in code, not the editor):
//   GameTraceChannel1 = Wall, GameTraceChannel2 = Threat, GameTraceChannel3 = Projectile,
//   GameTraceChannel4 = Pickup.
//
// Wall condition: Threatened once health drops past a threshold, Breached at zero.
UENUM(BlueprintType) // Lets Blueprints see this enum.
enum class EWallStatus : uint8 // A small integer-backed enum.
{
	Safe, // Plenty of health left.
	Threatened, // Health has dropped past the warning threshold.
	Breached // Health is zero: the wall is broken.
};

// Which of the four city walls an AWallActor represents.
UENUM(BlueprintType) // Lets Blueprints see this enum.
enum class EWallSide : uint8 // A small integer-backed enum.
{
	North, // The wall in front of the player's start.
	East, // The wall to the player's right.
	South, // The wall behind the player's start.
	West // The wall to the player's left.
};

// The three stations the player must keep running inside the walls (level 3).
UENUM(BlueprintType) // Lets Blueprints see this enum.
enum class ECityTaskKind : uint8 // A small integer-backed enum.
{
	Power, // The generator.
	Food, // The food store.
	Medic // The medical tent.
};

// How dangerous a threat is. Scales its damage and health.
UENUM(BlueprintType) // Lets Blueprints see this enum.
enum class EThreatSeverity : uint8 // A small integer-backed enum.
{
	Low, // Weak.
	Medium, // Average.
	Critical // Dangerous.
};

// The kinds of resource the player collects and spends. Each has its own use.
UENUM(BlueprintType) // Lets Blueprints see this enum.
enum class EResourceType : uint8 // A small integer-backed enum.
{
	Intel, // Analysis data. Spent to Validate a scanned wall.
	Patch, // Repair parts. Spent to fix a breached wall.
	Food, // Rations. Spent to feed the survivors at the food store.
	Power, // Battery cells. Spent to restart the generator.
	Medical // Medical kits. Spent to treat survivors at the medical tent.
};

// How many resource types there are (used to size the inventory array).
constexpr int32 ZeroDayResourceTypeCount = 5;

// Display name of a wall side, used in HUD text and alerts.
inline const TCHAR* WallSideToString(EWallSide Side)
{
	switch (Side) // Pick the text for this side.
	{
	case EWallSide::North: return TEXT("North"); // Front.
	case EWallSide::East: return TEXT("East"); // Right.
	case EWallSide::South: return TEXT("South"); // Back.
	default: return TEXT("West"); // Left.
	}
}

// Display name of a station, used in HUD text and alerts.
inline const TCHAR* TaskKindToString(ECityTaskKind Kind)
{
	switch (Kind) // Pick the text for this station.
	{
	case ECityTaskKind::Power: return TEXT("Power"); // The generator.
	case ECityTaskKind::Food: return TEXT("Food"); // The food store.
	default: return TEXT("Medical"); // The medical tent.
	}
}

// Display name of a threat severity, used in HUD text and alerts.
inline const TCHAR* SeverityToString(EThreatSeverity Severity)
{
	switch (Severity) // Pick the text for this severity.
	{
	case EThreatSeverity::Low: return TEXT("Low"); // Weak.
	case EThreatSeverity::Medium: return TEXT("Medium"); // Average.
	default: return TEXT("Critical"); // Dangerous.
	}
}

// Display name of a resource, used in HUD text and alerts.
inline const TCHAR* ResourceTypeToString(EResourceType Type)
{
	switch (Type) // Pick the text for this resource.
	{
	case EResourceType::Intel: return TEXT("Intel"); // Analysis data.
	case EResourceType::Patch: return TEXT("Patch"); // Repair parts.
	case EResourceType::Food: return TEXT("Food"); // Rations.
	case EResourceType::Power: return TEXT("Power"); // Battery cells.
	default: return TEXT("Medical"); // Medical kits.
	}
}

// Which resource a station needs to be attended.
inline EResourceType ResourceForTask(ECityTaskKind Kind)
{
	switch (Kind) // Each station uses its own resource.
	{
	case ECityTaskKind::Power: return EResourceType::Power; // Generator needs battery cells.
	case ECityTaskKind::Food: return EResourceType::Food; // Food store needs rations.
	default: return EResourceType::Medical; // Medical tent needs medical kits.
	}
}
