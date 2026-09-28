#include "ZeroDayHUD.h" // This class's own declaration.
#include "ZeroDayGameMode.h" // Where the data we draw comes from.
#include "WallActor.h" // Wall health and status.
#include "CityTaskActor.h" // Station condition.
#include "ThreatPawn.h" // Threat markers and health bars.
#include "DetectionComponent.h" // The scan cooldown.
#include "ZeroDayTypes.h" // Enums and name helpers.
#include "Engine/Canvas.h" // The drawing surface (SizeX / SizeY).
#include "Engine/Texture2D.h" // The icon pictures.
#include "UObject/ConstructorHelpers.h" // Lets a constructor load content assets.
#include "Engine/Engine.h" // GEngine, for the default font.
#include "EngineUtils.h" // TActorIterator.

namespace // Helpers and constants only this file uses.
{
	const FLinearColor PanelColor(0.02f, 0.03f, 0.05f, 0.7f); // Dark translucent panel background.
	const FLinearColor BarBackground(0.15f, 0.15f, 0.17f, 0.9f); // Empty part of a bar.
	const FLinearColor TextColor(0.95f, 0.95f, 0.97f, 1.f); // Main text.
	const FLinearColor MutedColor(0.65f, 0.67f, 0.72f, 1.f); // Secondary text.
	const FLinearColor GoodColor(0.3f, 0.85f, 0.4f, 1.f); // Healthy / success.
	const FLinearColor WarnColor(1.f, 0.7f, 0.15f, 1.f); // Warning.
	const FLinearColor BadColor(0.95f, 0.25f, 0.2f, 1.f); // Danger / failure.

	constexpr float TopBarHeight = 34.f; // Height of the bar across the top.
	constexpr float ObjectiveShowSeconds = 10.f; // How long the start-of-level goal stays up.

	// Colour for a threat severity label.
	FLinearColor SeverityColor(EThreatSeverity Severity)
	{
		switch (Severity) // Choose by severity.
		{
		case EThreatSeverity::Low: return FLinearColor(1.f, 0.9f, 0.3f, 1.f); // Yellow.
		case EThreatSeverity::Medium: return WarnColor; // Orange.
		default: return BadColor; // Red.
		}
	}

	// Green when healthy, amber when low, red when critical.
	FLinearColor HealthColor(float Fraction)
	{
		return Fraction > 0.5f ? GoodColor : (Fraction > 0.25f ? WarnColor : BadColor); // Two thresholds, three colours.
	}

	// Each resource has its own colour so it can be recognised at a glance.
	FLinearColor ResourceColor(EResourceType Type)
	{
		switch (Type) // Choose by resource.
		{
		case EResourceType::Intel: return FLinearColor(0.3f, 0.8f, 1.f, 1.f); // Cyan data.
		case EResourceType::Patch: return FLinearColor(1.f, 0.6f, 0.2f, 1.f); // Orange repair parts.
		case EResourceType::Food: return FLinearColor(0.55f, 0.85f, 0.35f, 1.f); // Green rations.
		case EResourceType::Power: return FLinearColor(1.f, 0.9f, 0.3f, 1.f); // Yellow battery.
		default: return FLinearColor(1.f, 0.4f, 0.55f, 1.f); // Pink medical.
		}
	}

	// One letter shown on each resource's icon tile.
	const TCHAR* ResourceLetter(EResourceType Type)
	{
		switch (Type) // Choose by resource.
		{
		case EResourceType::Intel: return TEXT("I"); // Intel.
		case EResourceType::Patch: return TEXT("P"); // Patch.
		case EResourceType::Food: return TEXT("F"); // Food.
		case EResourceType::Power: return TEXT("B"); // Battery.
		default: return TEXT("M"); // Medical.
		}
	}

	// What each resource is used for, shown next to its count.
	const TCHAR* ResourceUse(EResourceType Type)
	{
		switch (Type) // Choose by resource.
		{
		case EResourceType::Intel: return TEXT("Validate (F)"); // Spent by the Validate ability.
		case EResourceType::Patch: return TEXT("Fix a wall (E)"); // Spent when fixing a breach.
		case EResourceType::Food: return TEXT("Food store (E)"); // Spent at the food station.
		case EResourceType::Power: return TEXT("Generator (E)"); // Spent at the power station.
		default: return TEXT("Medical tent (E)"); // Spent at the medical station.
		}
	}
}

// Loads the icon pictures once. If any are missing the HUD falls back to coloured tiles.
AZeroDayHUD::AZeroDayHUD()
{
	static const TCHAR* IconPaths[ZeroDayResourceTypeCount] = { // In EResourceType order.
		TEXT("/Game/Sprites/T_pickup_intel.T_pickup_intel"),
		TEXT("/Game/Sprites/T_pickup_patch.T_pickup_patch"),
		TEXT("/Game/Sprites/T_pickup_food.T_pickup_food"),
		TEXT("/Game/Sprites/T_pickup_power.T_pickup_power"),
		TEXT("/Game/Sprites/T_pickup_medical.T_pickup_medical"),
	};
	ResourceIcons.SetNum(ZeroDayResourceTypeCount); // One slot per resource type.
	for (int32 Index = 0; Index < ZeroDayResourceTypeCount; ++Index) // Load each icon.
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> Finder(IconPaths[Index]); // Look it up.
		if (Finder.Succeeded()) // Found it.
		{
			ResourceIcons[Index] = Finder.Object; // Remember it.
		}
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> SurvivorFinder(TEXT("/Game/Sprites/T_icon_survivor.T_icon_survivor")); // The survivor picture.
	if (SurvivorFinder.Succeeded()) // Found it.
	{
		SurvivorIcon = SurvivorFinder.Object; // Remember it.
	}
}

// Called by the engine every frame. Draw order matters: world markers first so the panels sit on top.
void AZeroDayHUD::DrawHUD()
{
	Super::DrawHUD(); // Let the base class do its own drawing first.

	AZeroDayGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AZeroDayGameMode>() : nullptr; // Find our game mode.
	if (!GameMode || !Canvas) // Nothing to draw without both.
	{
		return; // Skip this frame.
	}

	DrawEffects(*GameMode); // Rings sit underneath everything else.
	DrawWorldHealthBars(*GameMode); // Bars over threats, walls and stations.
	DrawThreatMarkers(); // Boxes on scanned threats.
	DrawWallMarkers(*GameMode); // Red "!" on exposed walls.

	DrawTopBar(*GameMode); // Title, timer, survivors.
	DrawResourcePanel(*GameMode); // What the player carries.
	DrawStatusPanel(*GameMode); // Wall and station rows.
	DrawAlerts(*GameMode); // Fading messages.
	DrawControlsHint(); // Key reminder.

	if (GameMode->IsGameOver()) // Level finished?
	{
		DrawEndScreen(*GameMode); // Show the result and what to press next.
	}
	else
	{
		DrawObjectiveBanner(*GameMode); // Show the goal for the first few seconds.
	}
}

// Measures the text so it can be centred horizontally.
void AZeroDayHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, float Scale)
{
	float Width = 0.f; // Filled in by GetTextSize.
	float Height = 0.f; // Filled in by GetTextSize.
	GetTextSize(Text, Width, Height, GEngine->GetMediumFont(), Scale); // Measure the text.
	DrawText(Text, Color, (Canvas->SizeX - Width) * 0.5f, Y, GEngine->GetMediumFont(), Scale, false); // Draw it in the middle.
}

// Four lines make a hollow square (the canvas has no outline rectangle).
void AZeroDayHUD::DrawSquare(float CenterX, float CenterY, float HalfSize, const FLinearColor& Color)
{
	DrawLine(CenterX - HalfSize, CenterY - HalfSize, CenterX + HalfSize, CenterY - HalfSize, Color, 2.f); // Top edge.
	DrawLine(CenterX + HalfSize, CenterY - HalfSize, CenterX + HalfSize, CenterY + HalfSize, Color, 2.f); // Right edge.
	DrawLine(CenterX + HalfSize, CenterY + HalfSize, CenterX - HalfSize, CenterY + HalfSize, Color, 2.f); // Bottom edge.
	DrawLine(CenterX - HalfSize, CenterY + HalfSize, CenterX - HalfSize, CenterY - HalfSize, Color, 2.f); // Left edge.
}

// A ring is a polygon with enough sides to look round.
void AZeroDayHUD::DrawRing(float CenterX, float CenterY, float Radius, const FLinearColor& Color)
{
	constexpr int32 Segments = 28; // More segments means a smoother circle.
	for (int32 Index = 0; Index < Segments; ++Index) // One straight line per segment.
	{
		const float AngleA = 2.f * PI * static_cast<float>(Index) / Segments; // Start angle of this segment.
		const float AngleB = 2.f * PI * static_cast<float>(Index + 1) / Segments; // End angle of this segment.
		DrawLine(CenterX + FMath::Cos(AngleA) * Radius, CenterY + FMath::Sin(AngleA) * Radius, CenterX + FMath::Cos(AngleB) * Radius, CenterY + FMath::Sin(AngleB) * Radius, Color, 3.f); // Join the two points.
	}
}

// An empty bar with a coloured fill drawn on top of it.
void AZeroDayHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor)
{
	DrawRect(BarBackground, X, Y, Width, Height); // The empty track.
	DrawRect(FillColor, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height); // The filled part.
}

// A coloured tile with the resource's initial letter on it.
void AZeroDayHUD::DrawResourceIcon(EResourceType Type, float X, float Y, float Size)
{
	UTexture2D* Icon = ResourceIcons.IsValidIndex(static_cast<int32>(Type)) ? ResourceIcons[static_cast<int32>(Type)].Get() : nullptr; // The picture, if loaded.
	if (Icon) // We have artwork.
	{
		DrawTexture(Icon, X, Y, Size, Size, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent); // Draw the whole picture into the square.
		return; // Done.
	}
	DrawRect(ResourceColor(Type), X, Y, Size, Size); // Otherwise a coloured tile...
	DrawText(ResourceLetter(Type), FLinearColor::Black, X + Size * 0.28f, Y + Size * 0.08f, GEngine->GetMediumFont(), 0.8f, false); // ...with the letter, roughly centred.
}

// Level name, time left to hold out, and (on levels with stations) the survivors.
void AZeroDayHUD::DrawTopBar(const AZeroDayGameMode& GameMode)
{
	DrawRect(PanelColor, 0.f, 0.f, Canvas->SizeX, TopBarHeight); // Background strip across the top.

	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.
	DrawText(GameMode.GetLevelConfig().Title.ToUpper(), TextColor, 12.f, 8.f, Font, 1.f, false); // Level name, e.g. "LEVEL 2: VALIDATE".

	const int32 SecondsLeft = FMath::CeilToInt(GameMode.GetLevelTimeRemaining()); // Whole seconds remaining.
	const FString TimeText = FString::Printf(TEXT("Hold out: %d:%02d"), SecondsLeft / 60, SecondsLeft % 60); // Minutes:seconds.
	DrawCenteredText(TimeText, SecondsLeft <= 15 ? WarnColor : TextColor, 8.f); // Turns amber for the last fifteen seconds.

	if (GameMode.GetLevelConfig().bTasksActive) // Survivors only matter when stations can fail.
	{
		constexpr float IconSize = 22.f; // Each survivor is a small icon.
		constexpr float IconGap = 4.f; // Space between icons.
		const int32 Max = GameMode.GetMaxSurvivors(); // How many started.
		const int32 Alive = GameMode.GetSurvivors(); // How many are left.
		const float TotalWidth = Max * IconSize + (Max - 1) * IconGap; // Width of the whole row.
		float X = Canvas->SizeX - TotalWidth - 12.f; // Start so the row ends 12px from the right edge.
		for (int32 Index = 0; Index < Max; ++Index) // One square per survivor.
		{
			if (SurvivorIcon) // We have the picture.
			{
				DrawTexture(SurvivorIcon, X, 6.f, IconSize, IconSize, 0.f, 0.f, 1.f, 1.f, Index < Alive ? FLinearColor::White : FLinearColor(0.2f, 0.2f, 0.25f, 0.7f), BLEND_Translucent); // Full colour if alive, dim grey if lost.
			}
			else
			{
				DrawRect(Index < Alive ? GoodColor : FLinearColor(0.35f, 0.08f, 0.08f, 1.f), X, 10.f, IconSize, IconSize); // Otherwise a green square if alive, dark red if lost.
			}
			X += IconSize + IconGap; // Move to the next square.
		}
		float LabelWidth = 0.f; // Filled in by GetTextSize.
		float LabelHeight = 0.f; // Filled in by GetTextSize.
		GetTextSize(TEXT("Survivors"), LabelWidth, LabelHeight, Font, 0.85f); // Measure the label.
		DrawText(TEXT("Survivors"), MutedColor, Canvas->SizeX - TotalWidth - 24.f - LabelWidth, 9.f, Font, 0.85f, false); // Put it just left of the row.
	}
}

// Shows each resource the player is carrying and what it is for, so the choices are clear.
void AZeroDayHUD::DrawResourcePanel(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.
	const bool bStations = GameMode.GetLevelConfig().bTasksActive; // Food/Power/Medical only matter with stations.
	const int32 RowCount = bStations ? ZeroDayResourceTypeCount : 2; // Intel and Patch always, the rest on level 3.

	constexpr float RowHeight = 30.f; // Height of one row.
	constexpr float PanelWidth = 220.f; // Width of the panel.
	const float PanelX = Canvas->SizeX - PanelWidth - 12.f; // Sit against the right edge.
	const float PanelY = TopBarHeight + 10.f; // Just under the top bar.
	DrawRect(PanelColor, PanelX, PanelY, PanelWidth, RowCount * RowHeight + 12.f); // Panel background.

	float Y = PanelY + 8.f; // Top of the first row.
	for (int32 Index = 0; Index < RowCount; ++Index) // One row per visible resource.
	{
		const EResourceType Type = static_cast<EResourceType>(Index); // Which resource this row shows.
		const int32 Count = GameMode.GetResource(Type); // How many the player has.

		DrawResourceIcon(Type, PanelX + 8.f, Y, 22.f); // The icon.
		DrawText(FString::Printf(TEXT("%d"), Count), Count > 0 ? TextColor : BadColor, PanelX + 38.f, Y + 2.f, Font, 1.f, false); // The count, red at zero.
		DrawText(ResourceUse(Type), MutedColor, PanelX + 66.f, Y + 3.f, Font, 0.75f, false); // What it is spent on.
		Y += RowHeight; // Next row.
	}
}

// One row per active wall and station: name, health bar and status.
void AZeroDayHUD::DrawStatusPanel(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.
	constexpr float PanelX = 12.f; // Left margin.
	constexpr float RowHeight = 24.f; // Height of one row.
	constexpr float LabelWidth = 96.f; // Space for the name.
	constexpr float BarWidth = 110.f; // Width of each bar.
	constexpr float PanelWidth = 340.f; // Width of the panel.

	int32 Rows = 0; // How many rows we need.
	for (const AWallActor* Wall : GameMode.GetWalls()) // Count the walls in play.
	{
		Rows += (Wall && Wall->bIsActiveThisLevel) ? 1 : 0; // One row per active wall.
	}
	for (const ACityTaskActor* Task : GameMode.GetTasks()) // Count the stations in play.
	{
		Rows += (Task && Task->bIsActiveThisLevel) ? 1 : 0; // One row per active station.
	}
	if (Rows == 0) // Nothing to show.
	{
		return; // Skip the panel.
	}

	const float PanelY = TopBarHeight + 10.f; // Just under the top bar.
	DrawRect(PanelColor, PanelX, PanelY, PanelWidth, Rows * RowHeight + 12.f); // Panel background.

	float Y = PanelY + 8.f; // Top of the first row.
	for (const AWallActor* Wall : GameMode.GetWalls()) // Wall rows.
	{
		if (!Wall || !Wall->bIsActiveThisLevel) // Skip walls not in play.
		{
			continue; // Next wall.
		}

		const float Health = Wall->GetHealthPercent(); // 0-1 health.
		DrawText(FString::Printf(TEXT("%s wall"), WallSideToString(Wall->WallSide)), TextColor, PanelX + 8.f, Y, Font, 0.85f, false); // "North wall".
		DrawBar(PanelX + LabelWidth, Y + 3.f, BarWidth, 12.f, Health, HealthColor(Health)); // Health bar.

		FString StatusText = TEXT("Secure"); // Default status.
		FLinearColor StatusColor = GoodColor; // Default colour.
		if (Wall->GetStatus() == EWallStatus::Threatened) // Damaged?
		{
			StatusText = TEXT("Under attack"); // Warn the player.
			StatusColor = WarnColor; // Amber.
		}
		else if (Wall->GetStatus() == EWallStatus::Breached) // Broken?
		{
			StatusText = FString::Printf(TEXT("BREACHED %.0fs"), Wall->GetRemediationTimeRemaining()); // Show the countdown.
			StatusColor = BadColor; // Red.
		}
		DrawText(StatusText, StatusColor, PanelX + LabelWidth + BarWidth + 10.f, Y, Font, 0.85f, false); // Status text after the bar.
		Y += RowHeight; // Next row.
	}

	for (const ACityTaskActor* Task : GameMode.GetTasks()) // Station rows.
	{
		if (!Task || !Task->bIsActiveThisLevel) // Skip stations not in play.
		{
			continue; // Next station.
		}

		const float Condition = Task->GetCondition() / 100.f; // 0-1 condition.
		const EResourceType Need = ResourceForTask(Task->TaskKind); // What it costs to help.
		DrawText(TaskKindToString(Task->TaskKind), TextColor, PanelX + 8.f, Y, Font, 0.85f, false); // Station name.
		DrawBar(PanelX + LabelWidth, Y + 3.f, BarWidth, 12.f, Condition, HealthColor(Condition)); // Condition bar.
		const bool bCritical = Condition <= 0.25f; // Nearly failed?
		const FString StatusText = bCritical ? FString::Printf(TEXT("NEEDS %s!"), *FString(ResourceTypeToString(Need)).ToUpper()) : FString(TEXT("Running")); // Tell the player what to bring.
		DrawText(StatusText, bCritical ? BadColor : GoodColor, PanelX + LabelWidth + BarWidth + 10.f, Y, Font, 0.85f, false); // Status text after the bar.
		Y += RowHeight; // Next row.
	}
}

// Boxes around scanned threats. Unvalidated ones show '?', validated ones show severity or FALSE POSITIVE.
void AZeroDayHUD::DrawThreatMarkers()
{
	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.

	for (TActorIterator<AThreatPawn> It(GetWorld()); It; ++It) // Every threat in the world.
	{
		const AThreatPawn* Threat = *It; // The current threat.
		if (!Threat->IsDetected()) // Unscanned threats get no marker.
		{
			continue; // Next threat.
		}

		// Detected but not yet validated: we know something is there, not what it is.
		FString Label = TEXT("?"); // Unknown identity.
		FLinearColor Color = WarnColor; // Amber for "unknown".
		if (Threat->IsValidated()) // Validation reveals the truth.
		{
			Label = Threat->bIsDecoy ? FString(TEXT("FALSE POSITIVE")) : FString(SeverityToString(Threat->Severity)).ToUpper(); // Name it.
			Color = Threat->bIsDecoy ? MutedColor : SeverityColor(Threat->Severity); // Grey for harmless, severity colour for real.
		}

		const FVector Screen = Project(Threat->GetActorLocation()); // World position to screen position.
		DrawSquare(Screen.X, Screen.Y, 22.f, Color); // Box around the threat.

		float Width = 0.f; // Filled in by GetTextSize.
		float Height = 0.f; // Filled in by GetTextSize.
		GetTextSize(Label, Width, Height, Font, 0.8f); // Measure the label.
		DrawText(Label, Color, Screen.X - Width * 0.5f, Screen.Y - 44.f, Font, 0.8f, false); // Put it above the box.
	}
}

// Small health bars above threats, walls and stations so damage and dying are easy to read.
void AZeroDayHUD::DrawWorldHealthBars(const AZeroDayGameMode& GameMode)
{
	for (TActorIterator<AThreatPawn> It(GetWorld()); It; ++It) // Every threat.
	{
		const AThreatPawn* Threat = *It; // The current threat.
		const FVector Screen = Project(Threat->GetActorLocation()); // Screen position.
		DrawBar(Screen.X - 22.f, Screen.Y + 30.f, 44.f, 5.f, Threat->GetHealthFraction(), HealthColor(Threat->GetHealthFraction())); // Fraction only, so severity stays hidden.
	}

	for (const AWallActor* Wall : GameMode.GetWalls()) // Every wall.
	{
		if (!Wall || !Wall->bIsActiveThisLevel) // Skip walls not in play.
		{
			continue; // Next wall.
		}
		const FVector Screen = Project(Wall->GetActorLocation()); // Screen position.
		DrawBar(Screen.X - 60.f, Screen.Y - 62.f, 120.f, 7.f, Wall->GetHealthPercent(), HealthColor(Wall->GetHealthPercent())); // Wall health.
	}

	for (const ACityTaskActor* Task : GameMode.GetTasks()) // Every station.
	{
		if (!Task || !Task->bIsActiveThisLevel) // Skip stations not in play.
		{
			continue; // Next station.
		}
		const float Condition = Task->GetCondition() / 100.f; // 0-1 condition.
		const FVector Screen = Project(Task->GetActorLocation()); // Screen position.
		DrawBar(Screen.X - 35.f, Screen.Y - 62.f, 70.f, 7.f, Condition, HealthColor(Condition)); // Station condition.
	}
}

// Red '!' on walls with detected exposure, pinned to the screen edge if the wall is off camera.
void AZeroDayHUD::DrawWallMarkers(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.
	constexpr float Margin = 40.f; // Keep markers this far from the screen edge.

	// Pulse so the marker draws the eye.
	const float Pulse = 0.65f + 0.35f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f); // Oscillates between 0.3 and 1.0.

	for (const AWallActor* Wall : GameMode.GetWalls()) // Every wall.
	{
		if (!Wall || !Wall->bIsActiveThisLevel || !Wall->IsExposureDetected()) // Only scanned, active walls.
		{
			continue; // Next wall.
		}

		// Keep the marker on screen even when the wall is outside the camera view.
		const FVector Screen = Project(Wall->GetActorLocation()); // Screen position (may be off screen).
		const float X = FMath::Clamp(Screen.X, Margin, Canvas->SizeX - Margin); // Clamp horizontally.
		const float Y = FMath::Clamp(Screen.Y, TopBarHeight + Margin, Canvas->SizeY - Margin); // Clamp vertically.

		FLinearColor Color = BadColor; // Start from red.
		Color.A = Pulse; // Fade in and out.
		DrawRect(Color, X - 14.f, Y - 14.f, 28.f, 28.f); // Red square.
		DrawText(TEXT("!"), TextColor, X - 4.f, Y - 11.f, Font, 1.f, false); // Exclamation mark on it.

		const FString Label = FString::Printf(TEXT("%s exposed"), WallSideToString(Wall->WallSide)); // "North exposed".
		float Width = 0.f; // Filled in by GetTextSize.
		float Height = 0.f; // Filled in by GetTextSize.
		GetTextSize(Label, Width, Height, Font, 0.8f); // Measure the label.
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), X - Width * 0.5f - 6.f, Y + 17.f, Width + 12.f, Height + 2.f); // Dark backing so it reads over the wall.
		DrawText(Label, FLinearColor(1.f, 0.45f, 0.4f, 1.f), X - Width * 0.5f, Y + 18.f, Font, 0.8f, false); // Put it under the square in a brighter red.
	}
}

// Expanding, fading rings that show where something just happened.
void AZeroDayHUD::DrawEffects(const AZeroDayGameMode& GameMode)
{
	const float Now = GetWorld()->GetTimeSeconds(); // Current world time.
	for (const FZeroDayEffect& Effect : GameMode.GetEffects()) // Every ring that was queued.
	{
		const float Progress = (Now - Effect.StartTime) / FMath::Max(Effect.Duration, 0.01f); // 0 at the start, 1 at the end.
		if (Progress < 0.f || Progress > 1.f) // Not started or already finished.
		{
			continue; // Skip it.
		}

		const FVector Centre = Project(Effect.Location); // Ring centre on screen.
		const FVector Edge = Project(Effect.Location + FVector(Effect.MaxRadius * Progress, 0.f, 0.f)); // A point on the ring's edge, in the world.
		const float Radius = FMath::Max(6.f, FVector2D::Distance(FVector2D(Centre.X, Centre.Y), FVector2D(Edge.X, Edge.Y))); // Convert the world radius to pixels.

		FLinearColor Color = Effect.Color; // Start from the ring's colour.
		Color.A = 1.f - Progress; // Fade out as it grows.
		DrawRing(Centre.X, Centre.Y, Radius, Color); // Draw it.
	}
}

// Scan, validate and wall messages from the game mode, newest at the bottom.
void AZeroDayHUD::DrawAlerts(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont(); // The font used for all HUD text.
	const float Now = GetWorld()->GetTimeSeconds(); // Current world time.
	constexpr float LineHeight = 22.f; // Vertical space per message.

	// Newest alert at the bottom, older ones stack upwards and fade out.
	float Y = Canvas->SizeY - 70.f; // Bottom message position.
	const TArray<FZeroDayAlert>& Alerts = GameMode.GetAlerts(); // The message list.
	for (int32 Index = Alerts.Num() - 1; Index >= 0; --Index) // Newest first.
	{
		const FZeroDayAlert& Alert = Alerts[Index]; // The current message.
		const float Remaining = Alert.ExpireTime - Now; // Seconds until it expires.
		if (Remaining <= 0.f) // Already expired.
		{
			continue; // Skip it.
		}

		FLinearColor Color(Alert.Color); // Convert to a linear colour.
		Color.A = FMath::Clamp(Remaining / 1.5f, 0.f, 1.f); // Fade during the last second and a half.
		float TextWidth = 0.f; // Filled in by GetTextSize.
		float TextHeight = 0.f; // Filled in by GetTextSize.
		GetTextSize(Alert.Message, TextWidth, TextHeight, Font, 0.9f); // Measure the message so the backing fits it.
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f * Color.A), 10.f, Y - 2.f, TextWidth + 12.f, LineHeight); // Dark backing so the text is readable over the map.
		DrawText(Alert.Message, Color, 16.f, Y, Font, 0.9f, false); // Draw the message.
		Y -= LineHeight; // Move up for the next (older) message.
	}
}

// Key reminder along the bottom, including the Insta Scan cooldown.
void AZeroDayHUD::DrawControlsHint()
{
	FString ScanText = TEXT("Q Insta Scan: ready"); // Default text.
	if (const APawn* Pawn = GetOwningPawn()) // Find the player's pawn.
	{
		if (const UDetectionComponent* Detection = Pawn->FindComponentByClass<UDetectionComponent>()) // Find its detection tools.
		{
			const float Cooldown = Detection->GetScanCooldownRemaining(); // Seconds until the next scan.
			if (Cooldown > 0.f) // Still recharging?
			{
				ScanText = FString::Printf(TEXT("Q Insta Scan: %.0fs"), Cooldown); // Show the wait.
			}
		}
	}

	const FString Hint = FString::Printf(TEXT("WASD move   Click shoot   %s   F Validate   E Fix / Feed / Repair   R Restart"), *ScanText); // The full hint line.
	DrawRect(PanelColor, 0.f, Canvas->SizeY - 34.f, Canvas->SizeX, 34.f); // Background strip along the bottom.
	DrawCenteredText(Hint, MutedColor, Canvas->SizeY - 27.f, 0.85f); // Centred text.
}

// Start-of-level objective, so a first-time player knows what to do.
void AZeroDayHUD::DrawObjectiveBanner(const AZeroDayGameMode& GameMode)
{
	// Only show the objective at the start of a level so it doesn't clutter play.
	if (GameMode.GetTimeSinceLevelStart() > ObjectiveShowSeconds) // Measured from the start of this level.
	{
		return; // Hide it after a few seconds.
	}

	const float BannerY = Canvas->SizeY * 0.62f; // Low on the screen so it never covers the status panels at the top.
	DrawRect(PanelColor, 0.f, BannerY - 12.f, Canvas->SizeX, 72.f); // Dark strip behind the text.
	DrawCenteredText(TEXT("OBJECTIVE"), WarnColor, BannerY - 4.f, 0.9f); // Heading.
	DrawCenteredText(GameMode.GetLevelConfig().Objective, TextColor, BannerY + 24.f, 0.95f); // Each level has its own objective text.
}

// Level complete / game over overlay with what the player achieved and what to press next.
void AZeroDayHUD::DrawEndScreen(const AZeroDayGameMode& GameMode)
{
	const bool bWon = GameMode.DidPlayerWin(); // Did the player win this level?
	const FZeroDayLevelStats& Stats = GameMode.GetStats(); // What they achieved.

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY); // Darken the whole screen.

	const TCHAR* Headline = bWon ? (GameMode.IsFinalLevelComplete() ? TEXT("ALL LEVELS CLEARED") : TEXT("LEVEL COMPLETE")) : TEXT("CITY LOST"); // Pick the big title.
	const float Top = Canvas->SizeY * 0.28f; // Where the content starts.
	DrawCenteredText(Headline, bWon ? GoodColor : BadColor, Top, 3.f); // The headline.
	DrawCenteredText(GameMode.GetEndMessage(), TextColor, Top + 80.f, 1.1f); // Why it ended.

	// A short results list so the player sees what their choices achieved.
	float Y = Top + 130.f; // First results line.
	DrawCenteredText(FString::Printf(TEXT("Threats stopped: %d"), Stats.ThreatsStopped), TextColor, Y, 1.f); // Real threats shot.
	Y += 28.f; // Next line.
	DrawCenteredText(FString::Printf(TEXT("False positives identified: %d"), Stats.FalsePositivesIdentified), TextColor, Y, 1.f); // Decoys exposed by Validate.
	Y += 28.f; // Next line.
	DrawCenteredText(FString::Printf(TEXT("Walls patched: %d"), Stats.WallsFixed), TextColor, Y, 1.f); // Breaches repaired.
	Y += 28.f; // Next line.
	DrawCenteredText(FString::Printf(TEXT("Supplies collected: %d"), Stats.SuppliesCollected), TextColor, Y, 1.f); // Pickups the player gathered.
	if (GameMode.GetLevelConfig().bTasksActive) // Survivors only matter with stations.
	{
		Y += 28.f; // Next line.
		DrawCenteredText(FString::Printf(TEXT("Survivors saved: %d of %d"), GameMode.GetSurvivors(), GameMode.GetMaxSurvivors()), Stats.SurvivorsLost > 0 ? WarnColor : GoodColor, Y, 1.f); // How many lived.
	}

	FString Prompt = TEXT("Press R to try again"); // Default after losing.
	if (GameMode.CanAdvanceLevel()) // Won and there is another level to play.
	{
		Prompt = FString::Printf(TEXT("Press N for %s, or R to replay"), *FZeroDayLevelConfig::ForLevel(GameMode.CurrentLevel + 1).Title); // Name the next level.
	}
	else if (GameMode.IsFinalLevelComplete()) // Won the last level.
	{
		Prompt = TEXT("Press R to play again"); // Nothing after level 3.
	}
	DrawCenteredText(Prompt, WarnColor, Y + 60.f, 1.1f); // Tell the player what to do next.
}
