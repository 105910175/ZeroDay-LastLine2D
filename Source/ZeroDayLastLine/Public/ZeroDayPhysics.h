#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "ZeroDayTypes.h" // EResourceType.

class UPhysicalMaterial; // Forward declaration: Unreal's physics material asset type.

// Every physics material the game uses. Each one gives its object a different feel.
enum class EZeroDayMaterial : uint8 // Every physics material in the game.
{
	Wall, // Rough stone barrier.
	Threat, // Slippery shambling body.
	Projectile, // Frictionless, bouncy plasma bolt.
	Intel, // Light data chip: very bouncy.
	Patch, // Heavy metal parts: lands with a thud.
	Food, // Cardboard crate: grips and does not bounce.
	Power, // Battery cell: medium bounce.
	Medical, // Plastic case: springy.
	Count // Number of materials (not a real material).
};

// The numbers that define one physics material. These are the single source of truth: the
// asset generator writes them into the .uasset files, and the game falls back to them if a
// file is missing.
struct FZeroDayMaterialSpec // The numbers behind one material.
{
	const TCHAR* Name; // Asset name, e.g. "PM_Wall".
	float Friction; // How much it resists sliding (0 = ice, 1 = rubber).
	float Restitution; // How bouncy it is (0 = dead, 1 = perfectly elastic).
	float Density; // Weight per volume; heavier things push lighter things around.
	const TCHAR* Description; // What it is meant to feel like.
};

namespace ZeroDayPhysics // Helpers for looking up and creating the game's physics materials.
{
	// The defining numbers for a material.
	ZERODAYLASTLINE_API const FZeroDayMaterialSpec& GetSpec(EZeroDayMaterial Kind); // Looks up a material's numbers.

	// Which physics material a resource pickup uses.
	ZERODAYLASTLINE_API EZeroDayMaterial ForResource(EResourceType Type); // Which material a resource uses.

	// The content path of a material's asset, e.g. /Game/Physics/PM_Wall.PM_Wall.
	ZERODAYLASTLINE_API FString AssetPath(EZeroDayMaterial Kind); // Where its asset lives.

	// The material to put on a collider: the authored asset if it exists, otherwise an identical
	// one built from the spec. The result is cached and never null.
	ZERODAYLASTLINE_API UPhysicalMaterial* Get(EZeroDayMaterial Kind); // The material to put on a collider.

	// Writes a spec's numbers into an existing physics material.
	ZERODAYLASTLINE_API void ApplySpec(UPhysicalMaterial* Material, EZeroDayMaterial Kind); // Copies the numbers onto a material.

	// Builds a new physics material object from its spec (used by Get and by the asset generator).
	ZERODAYLASTLINE_API UPhysicalMaterial* CreateFromSpec(EZeroDayMaterial Kind, UObject* Outer, FName Name, EObjectFlags Flags);
}
