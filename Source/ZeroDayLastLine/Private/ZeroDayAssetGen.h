#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Commandlets/Commandlet.h" // Base class for command-line tools run by the editor.
#include "ZeroDayAssetGen.generated.h" // Unreal's generated reflection code (must be last include).

// Editor command-line tool that creates the game's Unreal assets:
//   * the physics materials (Content/Physics/PM_*), from the numbers in ZeroDayPhysics.cpp, and
//   * a texture and a Paper2D sprite (Content/Sprites/T_* and S_*) for every PNG in Art/Source.
// Run it (no graphics needed) with:
//   UnrealEditor-Cmd.exe ZeroDayLastLine.uproject -run=ZeroDayAssetGen -nullrhi -unattended
// It can be re-run at any time to refresh the assets after the art or the numbers change.
UCLASS() // Makes this class visible to Unreal's reflection system.
class UZeroDayAssetGenCommandlet : public UCommandlet // The engine finds it by the name ZeroDayAssetGen.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UZeroDayAssetGenCommandlet(); // Sets the commandlet up to run in the editor.

	virtual int32 Main(const FString& Params) override; // Does the work and returns 0 on success.
};
