#pragma once

#include "CoreMinimal.h"
#include "ZeroDayTypes.generated.h"

// Shared enums and small helpers used across the game's systems.
//
// Collision channels (set in code, not the editor):
//   GameTraceChannel1 = Wall, GameTraceChannel2 = Threat, GameTraceChannel3 = Projectile.
//
// Wall condition: Threatened once health drops past a threshold, Breached at zero.
UENUM(BlueprintType)
enum class EWallStatus : uint8
{
	Safe,
	Threatened,
	Breached
};

// Which of the four city walls an AWallActor represents.
UENUM(BlueprintType)
enum class EWallSide : uint8
{
	North,
	East,
	South,
	West
};

// The three tasks the player must keep running inside the walls (level 3).
UENUM(BlueprintType)
enum class ECityTaskKind : uint8
{
	Power,
	Food,
	Medic
};

// How dangerous a threat is. Scales its damage and health.
UENUM(BlueprintType)
enum class EThreatSeverity : uint8
{
	Low,
	Medium,
	Critical
};

// Display names used in HUD text and alerts.
inline const TCHAR* WallSideToString(EWallSide Side)
{
	switch (Side)
	{
	case EWallSide::North: return TEXT("North");
	case EWallSide::East: return TEXT("East");
	case EWallSide::South: return TEXT("South");
	default: return TEXT("West");
	}
}

inline const TCHAR* TaskKindToString(ECityTaskKind Kind)
{
	switch (Kind)
	{
	case ECityTaskKind::Power: return TEXT("Power");
	case ECityTaskKind::Food: return TEXT("Food");
	default: return TEXT("Medical");
	}
}

inline const TCHAR* SeverityToString(EThreatSeverity Severity)
{
	switch (Severity)
	{
	case EThreatSeverity::Low: return TEXT("Low");
	case EThreatSeverity::Medium: return TEXT("Medium");
	default: return TEXT("Critical");
	}
}
