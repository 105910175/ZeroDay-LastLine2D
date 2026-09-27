#pragma once

#include "CoreMinimal.h"
#include "ZeroDayTypes.generated.h"

UENUM(BlueprintType)
enum class EWallStatus : uint8
{
	Safe,
	Threatened,
	Breached
};

UENUM(BlueprintType)
enum class EWallSide : uint8
{
	North,
	East,
	South,
	West
};

UENUM(BlueprintType)
enum class ECityTaskKind : uint8
{
	Power,
	Food,
	Medic
};

UENUM(BlueprintType)
enum class EThreatSeverity : uint8
{
	Low,
	Medium,
	Critical
};

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

inline const TCHAR* SeverityToString(EThreatSeverity Severity)
{
	switch (Severity)
	{
	case EThreatSeverity::Low: return TEXT("Low");
	case EThreatSeverity::Medium: return TEXT("Medium");
	default: return TEXT("Critical");
	}
}
