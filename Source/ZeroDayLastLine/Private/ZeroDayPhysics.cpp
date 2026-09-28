#include "ZeroDayPhysics.h" // This file's own declarations.
#include "PhysicalMaterials/PhysicalMaterial.h" // Unreal's physics material class.
#include "UObject/UObjectGlobals.h" // NewObject, LoadObject.

namespace // Only this file uses these.
{
	// One row per EZeroDayMaterial, in the same order as the enum.
	const FZeroDayMaterialSpec Specs[] = {
		{ TEXT("PM_Wall"),       0.90f, 0.05f, 3.0f, TEXT("Stone: rough and dead, nothing bounces off it") },
		{ TEXT("PM_Threat"),     0.20f, 0.00f, 1.0f, TEXT("Shambling body: slippery, absorbs impacts") },
		{ TEXT("PM_Projectile"), 0.00f, 0.80f, 0.2f, TEXT("Plasma: frictionless and bouncy") },
		{ TEXT("PM_Intel"),      0.30f, 0.75f, 0.5f, TEXT("Data chip: light and very bouncy") },
		{ TEXT("PM_Patch"),      0.60f, 0.15f, 4.0f, TEXT("Metal parts: heavy, lands with a thud") },
		{ TEXT("PM_Food"),       0.85f, 0.05f, 1.0f, TEXT("Cardboard crate: grips the ground, no bounce") },
		{ TEXT("PM_Power"),      0.40f, 0.45f, 2.5f, TEXT("Battery cell: medium weight, medium bounce") },
		{ TEXT("PM_Medical"),    0.35f, 0.60f, 0.8f, TEXT("Plastic case: light and springy") },
	};
	static_assert(UE_ARRAY_COUNT(Specs) == static_cast<int32>(EZeroDayMaterial::Count), "One spec is needed for every material"); // Catch a missing row at compile time.

	UPhysicalMaterial* Cache[static_cast<int32>(EZeroDayMaterial::Count)] = { nullptr }; // Materials already looked up or built.
}

// The defining numbers for a material.
const FZeroDayMaterialSpec& ZeroDayPhysics::GetSpec(EZeroDayMaterial Kind)
{
	return Specs[static_cast<int32>(Kind)]; // Index the table by the enum value.
}

// Each resource is a different kind of object, so each gets its own material.
EZeroDayMaterial ZeroDayPhysics::ForResource(EResourceType Type)
{
	switch (Type) // Match the resource to its material.
	{
	case EResourceType::Intel: return EZeroDayMaterial::Intel; // Chip.
	case EResourceType::Patch: return EZeroDayMaterial::Patch; // Metal parts.
	case EResourceType::Food: return EZeroDayMaterial::Food; // Crate.
	case EResourceType::Power: return EZeroDayMaterial::Power; // Battery.
	default: return EZeroDayMaterial::Medical; // Medical case.
	}
}

// The path of the authored asset, in the form LoadObject expects.
FString ZeroDayPhysics::AssetPath(EZeroDayMaterial Kind)
{
	const FString Name = GetSpec(Kind).Name; // e.g. "PM_Wall".
	return FString::Printf(TEXT("/Game/Physics/%s.%s"), *Name, *Name); // /Game/Physics/PM_Wall.PM_Wall
}

// Copies a spec's numbers onto a material.
void ZeroDayPhysics::ApplySpec(UPhysicalMaterial* Material, EZeroDayMaterial Kind)
{
	const FZeroDayMaterialSpec& Spec = GetSpec(Kind); // The numbers to use.
	Material->Friction = Spec.Friction; // Sliding friction.
	Material->StaticFriction = Spec.Friction; // Resting friction matches, so it behaves consistently.
	Material->Restitution = Spec.Restitution; // Bounciness.
	Material->Density = Spec.Density; // Weight per volume.
}

// Builds a new material object holding a spec's numbers.
UPhysicalMaterial* ZeroDayPhysics::CreateFromSpec(EZeroDayMaterial Kind, UObject* Outer, FName Name, EObjectFlags Flags)
{
	UPhysicalMaterial* Material = NewObject<UPhysicalMaterial>(Outer, Name, Flags); // Create the object.
	ApplySpec(Material, Kind); // Fill in its numbers.
	return Material; // Hand it back.
}

// The material to put on a collider: the authored asset if there is one, otherwise an identical runtime copy.
UPhysicalMaterial* ZeroDayPhysics::Get(EZeroDayMaterial Kind)
{
	UPhysicalMaterial*& Slot = Cache[static_cast<int32>(Kind)]; // The cached entry for this material.
	if (!Slot) // First request?
	{
		UPhysicalMaterial* Loaded = LoadObject<UPhysicalMaterial>(nullptr, *AssetPath(Kind), nullptr, LOAD_NoWarn | LOAD_Quiet); // Try the authored asset first.
		Slot = Loaded ? Loaded : CreateFromSpec(Kind, GetTransientPackage(), NAME_None, RF_Transient); // Otherwise build one from the spec.
		Slot->AddToRoot(); // Keep it alive for the whole session so the cache never dangles.
	}
	return Slot; // Cached and never null.
}
