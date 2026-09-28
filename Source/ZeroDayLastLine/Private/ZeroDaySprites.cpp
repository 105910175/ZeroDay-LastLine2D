#include "ZeroDaySprites.h" // This file's own declaration.
#include "PaperSpriteComponent.h" // The component that draws a sprite.
#include "PaperSprite.h" // The sprite asset.

// Gives a sprite component its picture and lays it flat as pure decoration.
void ZeroDaySprites::ConfigureFlat(UPaperSpriteComponent* Component, UPaperSprite* Sprite, const FRotator& Rotation, float HeightOffset)
{
	if (!Component) // Nothing to configure.
	{
		return; // Bail out safely.
	}

	Component->SetSprite(Sprite); // Show this picture.
	Component->SetRelativeRotation(Rotation); // Lay it flat, facing up.
	Component->SetRelativeLocation(FVector(0.f, 0.f, HeightOffset)); // Sit just above the ground plane so it draws over the map.
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Decoration must never block or trigger anything.
	Component->SetGenerateOverlapEvents(false); // And should not create overlap events either.
	Component->SetCastShadow(false); // Flat art casts no shadow.
}
