#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.

class UPaperSpriteComponent; // Forward declaration: Paper2D's sprite-drawing component.
class UPaperSprite; // Forward declaration: Paper2D's sprite asset.

// Helpers for showing 2D sprites in a top-down game. Paper2D sprites stand upright by default,
// so they have to be laid flat to be seen from above.
namespace ZeroDaySprites
{
	// Lays a sprite flat with its art upright on the screen (image right = +Y, image up = +X).
	// Use for things that never turn, such as stations, pickups and walls' icons.
	inline FRotator UprightRotation()
	{
		return FRotationMatrix::MakeFromXZ(FVector(0.f, 1.f, 0.f), FVector(1.f, 0.f, 0.f)).Rotator(); // Right -> +Y, up -> +X.
	}

	// Lays a sprite flat with the art's right side pointing along the owner's forward (+X) axis.
	// Use for things that face the way they move, so the sprite turns with the actor.
	inline FRotator HeadingRotation()
	{
		return FRotationMatrix::MakeFromXZ(FVector(1.f, 0.f, 0.f), FVector(0.f, -1.f, 0.f)).Rotator(); // Right -> forward, up -> left of forward.
	}

	// Gives a sprite component a sprite, lays it flat, lifts it slightly above the ground plane
	// and makes sure it cannot collide with anything (sprites are decoration only).
	ZERODAYLASTLINE_API void ConfigureFlat(UPaperSpriteComponent* Component, UPaperSprite* Sprite, const FRotator& Rotation, float HeightOffset);
}
