#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZeroDayHUD.generated.h"

class AZeroDayGameMode;

// Draws all player-facing information straight onto the canvas (no widget assets
// needed): status panel, timer/resources, alerts, threat and wall markers, the
// level objective banner and the win/lose screen.
UCLASS()
class ZERODAYLASTLINE_API AZeroDayHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawTopBar(const AZeroDayGameMode& GameMode);
	void DrawStatusPanel(const AZeroDayGameMode& GameMode);
	void DrawThreatMarkers();
	void DrawWallMarkers(const AZeroDayGameMode& GameMode);
	void DrawAlerts(const AZeroDayGameMode& GameMode);
	void DrawControlsHint();
	void DrawObjectiveBanner(const AZeroDayGameMode& GameMode);
	void DrawEndScreen(const AZeroDayGameMode& GameMode);

	// Draws text horizontally centred on the screen at the given height.
	void DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, float Scale = 1.f);

	// Draws a hollow square around a screen position.
	void DrawSquare(float CenterX, float CenterY, float HalfSize, const FLinearColor& Color);
};
