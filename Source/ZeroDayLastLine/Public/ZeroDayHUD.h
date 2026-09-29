#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "GameFramework/HUD.h" // Base class for drawing 2D information on the screen.
#include "ZeroDayTypes.h" // EResourceType, ECityTaskKind.
#include "ZeroDayHUD.generated.h" // Unreal's generated reflection code (must be last include).

class AZeroDayGameMode; // Forward declaration: where the data we draw comes from.
class UTexture2D; // Forward declaration: the icon pictures.

// Draws all player-facing information straight onto the canvas (no widget assets needed):
// status panel, timer, resource counts, survivors, alerts, threat/wall markers, health
// bars, hit rings, the level objective banner and the level-complete / game-over screen.
UCLASS() // Makes this class visible to Unreal's reflection system.
class ZERODAYLASTLINE_API AZeroDayHUD : public AHUD // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	AZeroDayHUD(); // Loads the icon pictures.

	virtual void DrawHUD() override; // Called by the engine every frame to draw everything.

private:
	// One icon picture per resource type (indexed by EResourceType), and one for survivors.
	// Missing pictures are replaced by plain coloured tiles.
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> ResourceIcons;

	UPROPERTY()
	TObjectPtr<UTexture2D> SurvivorIcon;

	// A small wall-tile picture used as an icon next to each wall's row in the status panel.
	UPROPERTY()
	TObjectPtr<UTexture2D> WallIcon;

	// One icon per station kind (indexed by ECityTaskKind), used the same way.
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> StationIcons;

	void DrawTopBar(const AZeroDayGameMode& GameMode); // Title, timer and survivors.
	void DrawResourcePanel(const AZeroDayGameMode& GameMode); // What the player is carrying and what it is for.
	void DrawStatusPanel(const AZeroDayGameMode& GameMode); // Wall and station status rows.
	void DrawThreatMarkers(); // Boxes and labels on scanned threats.
	void DrawWorldHealthBars(const AZeroDayGameMode& GameMode); // Small bars over threats, walls and stations.
	void DrawWallMarkers(const AZeroDayGameMode& GameMode); // Red "!" on exposed walls.
	void DrawEffects(const AZeroDayGameMode& GameMode); // Expanding rings for hits, breaches and scans.
	void DrawAlerts(const AZeroDayGameMode& GameMode); // Fading messages at the bottom left.
	void DrawControlsHint(); // Key reminder along the bottom.
	void DrawObjectiveBanner(const AZeroDayGameMode& GameMode); // Start-of-level goal.
	void DrawEndScreen(const AZeroDayGameMode& GameMode); // Level complete / game over with results.

	// Draws text horizontally centred on the screen at the given height.
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, float Scale = 1.f);

	// Draws a hollow square around a screen position.
	void DrawSquare(float CenterX, float CenterY, float HalfSize, const FLinearColor& Color);

	// Draws a hollow circle around a screen position.
	void DrawRing(float CenterX, float CenterY, float Radius, const FLinearColor& Color);

	// Draws a horizontal bar that is filled to the given fraction (0-1), with gauge-style tick marks.
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor);

	// Draws one resource's small icon (a coloured tile with a letter) at the given position.
	void DrawResourceIcon(EResourceType Type, float X, float Y, float Size);

	// Draws a themed panel background: a dark fill, a thin accent border and small corner
	// brackets in the accent colour, so the HUD's boxes read as a designed interface rather
	// than a plain rectangle.
	void DrawPanelFrame(float X, float Y, float Width, float Height, const FLinearColor& Accent);

	// Draws part of a ring (a clockwise arc starting at 12 o'clock) covering Fraction of a
	// full circle. Used for the level timer and the Insta Scan cooldown.
	void DrawArc(float CenterX, float CenterY, float Radius, float Fraction, const FLinearColor& Color, float Thickness);
};
