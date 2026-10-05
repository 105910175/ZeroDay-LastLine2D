#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "ZeroDayAudioComponent.generated.h" // Unreal's generated reflection code (must be last include).

class USoundBase; // Forward declaration: any playable sound asset.

/**
 * Centralised sound-effect player for Zero Day: Last Line.
 *
 * The component loads sounds lazily from /Game/Audio/SFX so the project can still
 * compile and run while audio assets are being prepared. Missing sounds are
 * reported once in the log and otherwise ignored safely.
 */
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UZeroDayAudioComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UZeroDayAudioComponent(); // Turns ticking off; the component only reacts to calls.

	void PlayFire(const FVector& Location); // The player's shot leaving the weapon.
	void PlayProjectileImpact(const FVector& Location); // A shot hitting something solid.
	void PlayThreatDestroyed(const FVector& Location); // A threat being shot down.
	void PlayPickup(const FVector& Location); // The player collecting a resource.
	void PlayScan(); // Insta Scan pulse (non-positional).
	void PlayValidate(const FVector& Location); // Validate confirming a wall's threats.
	void PlayWallHit(const FVector& Location); // A threat striking a wall.
	void PlayWallBreached(const FVector& Location); // A wall's health reaching zero.
	void PlayWallRepaired(const FVector& Location); // A breached wall being patched.
	void PlayStationAttended(const FVector& Location); // A station being successfully attended.
	void PlayStationWarning(const FVector& Location); // A station dropping to critical.
	void PlayStationFailed(const FVector& Location); // A station running out of time.
	void PlaySupplyDrop(const FVector& Location); // A supply crate landing in the city.
	void PlaySurge(); // A level 3 zero-day surge starting (non-positional).
	void PlayLevelWon(); // The level being won (non-positional).
	void PlayLevelLost(); // The level being lost (non-positional).

private:
	// Sounds already loaded, keyed by asset path, so each file is only loaded once.
	UPROPERTY(Transient) // Kept as a UPROPERTY so the garbage collector does not free the sounds.
	TMap<FString, TObjectPtr<USoundBase>> CachedSounds; // Path -> loaded sound.

	TSet<FString> MissingSounds; // Paths that failed to load, so the warning only prints once.

	USoundBase* ResolveSound(const TCHAR* AssetPath); // Finds a sound by path (cached), or null if it does not exist.
	void Play2D(const TCHAR* AssetPath, float Volume = 1.f, float Pitch = 1.f); // Plays a sound with no position (UI-style).
	void PlayAtLocation(const TCHAR* AssetPath, const FVector& Location, float Volume = 1.f, float Pitch = 1.f); // Plays a sound from a spot in the world.
};
