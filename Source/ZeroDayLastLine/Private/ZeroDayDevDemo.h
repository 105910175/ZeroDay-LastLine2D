#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Subsystems/WorldSubsystem.h" // Base class for per-world helper objects.
#include "ZeroDayDevDemo.generated.h" // Unreal's generated reflection code (must be last include).

// Development tool: when the game is launched with -ZeroDayDemo it plays through a fixed
// script (scan, validate, level 3, breach, level complete) and saves a screenshot after each
// step. It never runs in normal play or in shipping builds.
UCLASS() // Makes this class visible to Unreal's reflection system.
class UZeroDayDevDemo : public UWorldSubsystem // Created automatically for each world.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override; // Only exists when -ZeroDayDemo is on the command line.
	virtual void OnWorldBeginPlay(UWorld& InWorld) override; // Starts the script once the game is running.

private:
	// Runs the next step of the script; called by a repeating timer.
	void RunNextStep();

	FTimerHandle StepTimer; // Drives the script.
	int32 StepIndex = 0; // Which step runs next.
};
