#include "ZeroDayHUD.h"
#include "ZeroDayGameMode.h"
#include "WallActor.h"
#include "CityTaskActor.h"
#include "ThreatPawn.h"
#include "DetectionComponent.h"
#include "ZeroDayTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

namespace
{
	const FLinearColor PanelColor(0.02f, 0.03f, 0.05f, 0.7f);
	const FLinearColor BarBackground(0.15f, 0.15f, 0.17f, 0.9f);
	const FLinearColor TextColor(0.95f, 0.95f, 0.97f, 1.f);
	const FLinearColor MutedColor(0.65f, 0.67f, 0.72f, 1.f);
	const FLinearColor GoodColor(0.3f, 0.85f, 0.4f, 1.f);
	const FLinearColor WarnColor(1.f, 0.7f, 0.15f, 1.f);
	const FLinearColor BadColor(0.95f, 0.25f, 0.2f, 1.f);

	constexpr float TopBarHeight = 34.f;
	constexpr float ObjectiveShowSeconds = 10.f;

	FLinearColor SeverityColor(EThreatSeverity Severity)
	{
		switch (Severity)
		{
		case EThreatSeverity::Low: return FLinearColor(1.f, 0.9f, 0.3f, 1.f);
		case EThreatSeverity::Medium: return WarnColor;
		default: return BadColor;
		}
	}

	// Green when healthy, amber when low, red when critical.
	FLinearColor HealthColor(float Fraction)
	{
		return Fraction > 0.5f ? GoodColor : (Fraction > 0.25f ? WarnColor : BadColor);
	}
}

// Draw order matters: world markers first so the panels sit on top of them.
void AZeroDayHUD::DrawHUD()
{
	Super::DrawHUD();

	AZeroDayGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AZeroDayGameMode>() : nullptr;
	if (!GameMode || !Canvas)
	{
		return;
	}

	// World-space markers first so the panels draw on top of them.
	DrawThreatMarkers();
	DrawWallMarkers(*GameMode);

	DrawTopBar(*GameMode);
	DrawStatusPanel(*GameMode);
	DrawAlerts(*GameMode);
	DrawControlsHint();

	if (GameMode->IsGameOver())
	{
		DrawEndScreen(*GameMode);
	}
	else
	{
		DrawObjectiveBanner(*GameMode);
	}
}

// Measures the text so it can be centred horizontally.
void AZeroDayHUD::DrawCenteredText(const FString& Text, const FLinearColor& Color, float Y, float Scale)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(Text, Width, Height, GEngine->GetMediumFont(), Scale);
	DrawText(Text, Color, (Canvas->SizeX - Width) * 0.5f, Y, GEngine->GetMediumFont(), Scale, false);
}

// Four lines make a hollow square (the canvas has no outline rectangle).
void AZeroDayHUD::DrawSquare(float CenterX, float CenterY, float HalfSize, const FLinearColor& Color)
{
	DrawLine(CenterX - HalfSize, CenterY - HalfSize, CenterX + HalfSize, CenterY - HalfSize, Color, 2.f);
	DrawLine(CenterX + HalfSize, CenterY - HalfSize, CenterX + HalfSize, CenterY + HalfSize, Color, 2.f);
	DrawLine(CenterX + HalfSize, CenterY + HalfSize, CenterX - HalfSize, CenterY + HalfSize, Color, 2.f);
	DrawLine(CenterX - HalfSize, CenterY + HalfSize, CenterX - HalfSize, CenterY - HalfSize, Color, 2.f);
}

// Level number, time left to hold out, and resources.
void AZeroDayHUD::DrawTopBar(const AZeroDayGameMode& GameMode)
{
	DrawRect(PanelColor, 0.f, 0.f, Canvas->SizeX, TopBarHeight);

	UFont* Font = GEngine->GetMediumFont();
	DrawText(FString::Printf(TEXT("LEVEL %d"), GameMode.CurrentLevel), TextColor, 12.f, 8.f, Font, 1.f, false);

	const int32 SecondsLeft = FMath::CeilToInt(GameMode.GetLevelTimeRemaining());
	const FString TimeText = FString::Printf(TEXT("Hold out: %d:%02d"), SecondsLeft / 60, SecondsLeft % 60);
	DrawCenteredText(TimeText, SecondsLeft <= 15 ? WarnColor : TextColor, 8.f);

	const FString ResourceText = FString::Printf(TEXT("Resources: %d"), GameMode.Resources);
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(ResourceText, Width, Height, Font, 1.f);
	DrawText(ResourceText, TextColor, Canvas->SizeX - Width - 12.f, 8.f, Font, 1.f, false);
}

// One row per active wall and city task: name, health bar and status.
void AZeroDayHUD::DrawStatusPanel(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont();
	constexpr float PanelX = 12.f;
	constexpr float RowHeight = 24.f;
	constexpr float LabelWidth = 96.f;
	constexpr float BarWidth = 110.f;
	constexpr float PanelWidth = 340.f;

	int32 Rows = 0;
	for (const AWallActor* Wall : GameMode.GetWalls())
	{
		Rows += (Wall && Wall->bIsActiveThisLevel) ? 1 : 0;
	}
	for (const ACityTaskActor* Task : GameMode.GetTasks())
	{
		Rows += (Task && Task->bIsActiveThisLevel) ? 1 : 0;
	}
	if (Rows == 0)
	{
		return;
	}

	const float PanelY = TopBarHeight + 10.f;
	DrawRect(PanelColor, PanelX, PanelY, PanelWidth, Rows * RowHeight + 12.f);

	float Y = PanelY + 8.f;
	for (const AWallActor* Wall : GameMode.GetWalls())
	{
		if (!Wall || !Wall->bIsActiveThisLevel)
		{
			continue;
		}

		const float Health = Wall->GetHealthPercent();
		DrawText(FString::Printf(TEXT("%s wall"), WallSideToString(Wall->WallSide)), TextColor, PanelX + 8.f, Y, Font, 0.85f, false);
		DrawRect(BarBackground, PanelX + LabelWidth, Y + 3.f, BarWidth, 12.f);
		DrawRect(HealthColor(Health), PanelX + LabelWidth, Y + 3.f, BarWidth * Health, 12.f);

		FString StatusText = TEXT("Secure");
		FLinearColor StatusColor = GoodColor;
		if (Wall->GetStatus() == EWallStatus::Threatened)
		{
			StatusText = TEXT("Under attack");
			StatusColor = WarnColor;
		}
		else if (Wall->GetStatus() == EWallStatus::Breached)
		{
			StatusText = FString::Printf(TEXT("BREACHED %.0fs"), Wall->GetRemediationTimeRemaining());
			StatusColor = BadColor;
		}
		DrawText(StatusText, StatusColor, PanelX + LabelWidth + BarWidth + 10.f, Y, Font, 0.85f, false);
		Y += RowHeight;
	}

	for (const ACityTaskActor* Task : GameMode.GetTasks())
	{
		if (!Task || !Task->bIsActiveThisLevel)
		{
			continue;
		}

		const float Condition = Task->GetCondition() / 100.f;
		DrawText(TaskKindToString(Task->TaskKind), TextColor, PanelX + 8.f, Y, Font, 0.85f, false);
		DrawRect(BarBackground, PanelX + LabelWidth, Y + 3.f, BarWidth, 12.f);
		DrawRect(HealthColor(Condition), PanelX + LabelWidth, Y + 3.f, BarWidth * Condition, 12.f);
		DrawText(Condition <= 0.25f ? TEXT("CRITICAL") : TEXT("Running"), Condition <= 0.25f ? BadColor : GoodColor,
			PanelX + LabelWidth + BarWidth + 10.f, Y, Font, 0.85f, false);
		Y += RowHeight;
	}
}

// Boxes around scanned threats. Unvalidated ones show '?', validated ones show severity or FALSE POSITIVE.
void AZeroDayHUD::DrawThreatMarkers()
{
	UFont* Font = GEngine->GetMediumFont();

	for (TActorIterator<AThreatPawn> It(GetWorld()); It; ++It)
	{
		const AThreatPawn* Threat = *It;
		if (!Threat->IsDetected())
		{
			continue;
		}

		// Detected but not yet validated: we know something is there, not what it is.
		FString Label = TEXT("?");
		FLinearColor Color = WarnColor;
		if (Threat->IsValidated())
		{
			Label = Threat->bIsDecoy ? FString(TEXT("FALSE POSITIVE")) : FString(SeverityToString(Threat->Severity)).ToUpper();
			Color = Threat->bIsDecoy ? MutedColor : SeverityColor(Threat->Severity);
		}

		const FVector Screen = Project(Threat->GetActorLocation());
		DrawSquare(Screen.X, Screen.Y, 22.f, Color);

		float Width = 0.f;
		float Height = 0.f;
		GetTextSize(Label, Width, Height, Font, 0.8f);
		DrawText(Label, Color, Screen.X - Width * 0.5f, Screen.Y - 44.f, Font, 0.8f, false);
	}
}

// Red '!' on walls with detected exposure, pinned to the screen edge if the wall is off camera.
void AZeroDayHUD::DrawWallMarkers(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont();
	constexpr float Margin = 40.f;

	// Pulse so the marker draws the eye.
	const float Pulse = 0.65f + 0.35f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f);

	for (const AWallActor* Wall : GameMode.GetWalls())
	{
		if (!Wall || !Wall->bIsActiveThisLevel || !Wall->IsExposureDetected())
		{
			continue;
		}

		// Keep the marker on screen even when the wall is outside the camera view.
		const FVector Screen = Project(Wall->GetActorLocation());
		const float X = FMath::Clamp(Screen.X, Margin, Canvas->SizeX - Margin);
		const float Y = FMath::Clamp(Screen.Y, TopBarHeight + Margin, Canvas->SizeY - Margin);

		FLinearColor Color = BadColor;
		Color.A = Pulse;
		DrawRect(Color, X - 14.f, Y - 14.f, 28.f, 28.f);
		DrawText(TEXT("!"), TextColor, X - 4.f, Y - 11.f, Font, 1.f, false);

		const FString Label = FString::Printf(TEXT("%s exposed"), WallSideToString(Wall->WallSide));
		float Width = 0.f;
		float Height = 0.f;
		GetTextSize(Label, Width, Height, Font, 0.8f);
		DrawText(Label, BadColor, X - Width * 0.5f, Y + 18.f, Font, 0.8f, false);
	}
}

// Scan, validate and wall messages from the game mode.
void AZeroDayHUD::DrawAlerts(const AZeroDayGameMode& GameMode)
{
	UFont* Font = GEngine->GetMediumFont();
	const float Now = GetWorld()->GetTimeSeconds();
	constexpr float LineHeight = 22.f;

	// Newest alert at the bottom, older ones stack upwards and fade out.
	float Y = Canvas->SizeY - 70.f;
	const TArray<FZeroDayAlert>& Alerts = GameMode.GetAlerts();
	for (int32 Index = Alerts.Num() - 1; Index >= 0; --Index)
	{
		const FZeroDayAlert& Alert = Alerts[Index];
		const float Remaining = Alert.ExpireTime - Now;
		if (Remaining <= 0.f)
		{
			continue;
		}

		FLinearColor Color(Alert.Color);
		Color.A = FMath::Clamp(Remaining / 1.5f, 0.f, 1.f);
		DrawText(Alert.Message, Color, 16.f, Y, Font, 0.9f, false);
		Y -= LineHeight;
	}
}

// Key reminder along the bottom, including the Insta Scan cooldown.
void AZeroDayHUD::DrawControlsHint()
{
	FString ScanText = TEXT("Q Insta Scan: ready");
	if (const APawn* Pawn = GetOwningPawn())
	{
		if (const UDetectionComponent* Detection = Pawn->FindComponentByClass<UDetectionComponent>())
		{
			const float Cooldown = Detection->GetScanCooldownRemaining();
			if (Cooldown > 0.f)
			{
				ScanText = FString::Printf(TEXT("Q Insta Scan: %.0fs"), Cooldown);
			}
		}
	}

	const FString Hint = FString::Printf(TEXT("WASD move   Click shoot   %s   F Validate   E Fix / Attend   R Restart"), *ScanText);
	DrawRect(PanelColor, 0.f, Canvas->SizeY - 34.f, Canvas->SizeX, 34.f);
	DrawCenteredText(Hint, MutedColor, Canvas->SizeY - 27.f, 0.85f);
}

// Start-of-level objective, so a first-time player knows what to do.
void AZeroDayHUD::DrawObjectiveBanner(const AZeroDayGameMode& GameMode)
{
	// Only show the objective at the start of a level so it doesn't clutter play.
	if (GetWorld()->GetTimeSeconds() > ObjectiveShowSeconds)
	{
		return;
	}

	DrawRect(PanelColor, 0.f, Canvas->SizeY * 0.3f - 12.f, Canvas->SizeX, 72.f);
	DrawCenteredText(TEXT("OBJECTIVE"), WarnColor, Canvas->SizeY * 0.3f - 4.f, 0.9f);
	DrawCenteredText(GameMode.GetObjectiveText(), TextColor, Canvas->SizeY * 0.3f + 24.f, 0.95f);
}

// Win/lose overlay with the reason and what to press next.
void AZeroDayHUD::DrawEndScreen(const AZeroDayGameMode& GameMode)
{
	const bool bWon = GameMode.DidPlayerWin();

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
	DrawCenteredText(bWon ? TEXT("CITY SAVED") : TEXT("CITY LOST"), bWon ? GoodColor : BadColor, Canvas->SizeY * 0.35f, 3.f);
	DrawCenteredText(GameMode.GetEndMessage(), TextColor, Canvas->SizeY * 0.35f + 80.f, 1.1f);

	FString Prompt = TEXT("Press R to try again");
	if (GameMode.CanLoadNextLevel())
	{
		Prompt = TEXT("Press N for the next level, or R to replay");
	}
	DrawCenteredText(Prompt, MutedColor, Canvas->SizeY * 0.35f + 120.f, 1.f);
}
