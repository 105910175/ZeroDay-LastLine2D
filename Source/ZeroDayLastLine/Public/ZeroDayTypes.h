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
