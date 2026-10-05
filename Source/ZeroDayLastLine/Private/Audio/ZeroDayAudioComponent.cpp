#include "Audio/ZeroDayAudioComponent.h" // This class's own declaration.

#include "Kismet/GameplayStatics.h" // PlaySound2D / PlaySoundAtLocation.
#include "Sound/SoundBase.h" // The sound asset type.
#include "UObject/UObjectGlobals.h" // LoadObject.

namespace ZeroDayAudioPaths // Only this file needs the content paths of every sound.
{
	constexpr const TCHAR* Fire = TEXT("/Game/Audio/SFX/SFX_Fire.SFX_Fire"); // Player shot.
	constexpr const TCHAR* ProjectileImpact = TEXT("/Game/Audio/SFX/SFX_ProjectileImpact.SFX_ProjectileImpact"); // Shot hits something.
	constexpr const TCHAR* ThreatDestroyed = TEXT("/Game/Audio/SFX/SFX_ThreatDestroyed.SFX_ThreatDestroyed"); // Threat dies.
	constexpr const TCHAR* Pickup = TEXT("/Game/Audio/SFX/SFX_Pickup.SFX_Pickup"); // Resource collected.
	constexpr const TCHAR* Scan = TEXT("/Game/Audio/SFX/SFX_Scan.SFX_Scan"); // Insta Scan.
	constexpr const TCHAR* Validate = TEXT("/Game/Audio/SFX/SFX_Validate.SFX_Validate"); // Validate.
	constexpr const TCHAR* WallHit = TEXT("/Game/Audio/SFX/SFX_WallHit.SFX_WallHit"); // Wall takes a hit.
	constexpr const TCHAR* WallBreached = TEXT("/Game/Audio/SFX/SFX_WallBreached.SFX_WallBreached"); // Wall breached.
	constexpr const TCHAR* WallRepaired = TEXT("/Game/Audio/SFX/SFX_WallRepaired.SFX_WallRepaired"); // Wall repaired.
	constexpr const TCHAR* StationAttended = TEXT("/Game/Audio/SFX/SFX_StationAttended.SFX_StationAttended"); // Station attended.
	constexpr const TCHAR* StationWarning = TEXT("/Game/Audio/SFX/SFX_StationWarning.SFX_StationWarning"); // Station critical.
	constexpr const TCHAR* StationFailed = TEXT("/Game/Audio/SFX/SFX_StationFailed.SFX_StationFailed"); // Station failed.
	constexpr const TCHAR* SupplyDrop = TEXT("/Game/Audio/SFX/SFX_SupplyDrop.SFX_SupplyDrop"); // Supply crate lands.
	constexpr const TCHAR* Surge = TEXT("/Game/Audio/SFX/SFX_Surge.SFX_Surge"); // Zero-day surge begins.
	constexpr const TCHAR* LevelWon = TEXT("/Game/Audio/SFX/SFX_LevelWon.SFX_LevelWon"); // Level won.
	constexpr const TCHAR* LevelLost = TEXT("/Game/Audio/SFX/SFX_LevelLost.SFX_LevelLost"); // Level lost.
}

// Nothing here needs to run every frame, so ticking is switched off.
UZeroDayAudioComponent::UZeroDayAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // The component only reacts to Play... calls.
}

// Finds a sound by path. Loaded sounds are cached; sounds that failed to load are remembered so they only warn once.
USoundBase* UZeroDayAudioComponent::ResolveSound(const TCHAR* AssetPath)
{
	const FString Path(AssetPath); // Use a string as the cache key.

	if (TObjectPtr<USoundBase>* Cached = CachedSounds.Find(Path)) // Already loaded earlier?
	{
		return Cached->Get(); // Reuse it.
	}

	if (MissingSounds.Contains(Path)) // Already known to be missing?
	{
		return nullptr; // Do not try (or warn) again.
	}

	if (USoundBase* Loaded = LoadObject<USoundBase>(nullptr, AssetPath)) // First request: try to load the asset.
	{
		CachedSounds.Add(Path, Loaded); // Remember it for next time.
		return Loaded; // Hand it back.
	}

	MissingSounds.Add(Path); // Remember that it failed.
	UE_LOG(LogTemp, Warning, TEXT("ZeroDayAudio: sound asset not found: %s"), AssetPath); // Tell the developer once.
	return nullptr; // Nothing to play.
}

// Plays a sound that does not come from a place in the world (it sounds the same everywhere).
void UZeroDayAudioComponent::Play2D(const TCHAR* AssetPath, float Volume, float Pitch)
{
	if (USoundBase* Sound = ResolveSound(AssetPath)) // Only if the sound exists.
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume, Pitch, 0.f, nullptr, nullptr, false); // Fire and forget.
	}
}

// Plays a sound from a world position, so it gets quieter with distance.
void UZeroDayAudioComponent::PlayAtLocation(const TCHAR* AssetPath, const FVector& Location, float Volume, float Pitch)
{
	if (USoundBase* Sound = ResolveSound(AssetPath)) // Only if the sound exists.
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Volume, Pitch, 0.f, nullptr, nullptr, nullptr); // Fire and forget.
	}
}

// The player's shot.
void UZeroDayAudioComponent::PlayFire(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Fire, Location, 0.55f, 1.f); // Quiet, so rapid fire is not tiring.
}

// A shot hitting something solid.
void UZeroDayAudioComponent::PlayProjectileImpact(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::ProjectileImpact, Location, 0.65f, FMath::FRandRange(0.95f, 1.05f)); // Slight random pitch avoids repetition.
}

// A threat dying.
void UZeroDayAudioComponent::PlayThreatDestroyed(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::ThreatDestroyed, Location, 0.8f, FMath::FRandRange(0.96f, 1.04f)); // Slight random pitch avoids repetition.
}

// Collecting a resource.
void UZeroDayAudioComponent::PlayPickup(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Pickup, Location, 0.7f, 1.f); // Plain pickup chime.
}

// Insta Scan.
void UZeroDayAudioComponent::PlayScan()
{
	Play2D(ZeroDayAudioPaths::Scan, 0.75f, 1.f); // The scan is not located anywhere, so play it flat.
}

// Validate.
void UZeroDayAudioComponent::PlayValidate(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Validate, Location, 0.75f, 1.f); // Comes from the wall that was validated.
}

// A threat striking a wall.
void UZeroDayAudioComponent::PlayWallHit(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallHit, Location, 0.65f, FMath::FRandRange(0.96f, 1.04f)); // Slight random pitch avoids repetition.
}

// A wall breaking.
void UZeroDayAudioComponent::PlayWallBreached(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallBreached, Location, 0.9f, 1.f); // Loud: this is an emergency.
}

// A wall being patched.
void UZeroDayAudioComponent::PlayWallRepaired(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallRepaired, Location, 0.8f, 1.f); // Reassuring repair sound.
}

// A station being attended.
void UZeroDayAudioComponent::PlayStationAttended(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationAttended, Location, 0.7f, 1.f); // Confirms the resource was spent.
}

// A station dropping to critical.
void UZeroDayAudioComponent::PlayStationWarning(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationWarning, Location, 0.65f, 1.f); // Warns the player to go there.
}

// A station failing.
void UZeroDayAudioComponent::PlayStationFailed(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationFailed, Location, 0.85f, 1.f); // A survivor was lost.
}

// A supply crate landing.
void UZeroDayAudioComponent::PlaySupplyDrop(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::SupplyDrop, Location, 0.8f, 1.f); // Tells the player loot arrived.
}

// A zero-day surge starting.
void UZeroDayAudioComponent::PlaySurge()
{
	Play2D(ZeroDayAudioPaths::Surge, 0.9f, 1.f); // Not tied to a place: it is a warning to the player.
}

// Winning a level.
void UZeroDayAudioComponent::PlayLevelWon()
{
	Play2D(ZeroDayAudioPaths::LevelWon, 1.f, 1.f); // Full volume fanfare.
}

// Losing a level.
void UZeroDayAudioComponent::PlayLevelLost()
{
	Play2D(ZeroDayAudioPaths::LevelLost, 1.f, 1.f); // Full volume defeat sting.
}
