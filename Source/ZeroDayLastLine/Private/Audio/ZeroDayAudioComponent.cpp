#include "Audio/ZeroDayAudioComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"

namespace ZeroDayAudioPaths
{
	constexpr const TCHAR* Fire = TEXT("/Game/Audio/SFX/SFX_Fire.SFX_Fire");
	constexpr const TCHAR* ProjectileImpact = TEXT("/Game/Audio/SFX/SFX_ProjectileImpact.SFX_ProjectileImpact");
	constexpr const TCHAR* ThreatDestroyed = TEXT("/Game/Audio/SFX/SFX_ThreatDestroyed.SFX_ThreatDestroyed");
	constexpr const TCHAR* Pickup = TEXT("/Game/Audio/SFX/SFX_Pickup.SFX_Pickup");
	constexpr const TCHAR* Scan = TEXT("/Game/Audio/SFX/SFX_Scan.SFX_Scan");
	constexpr const TCHAR* Validate = TEXT("/Game/Audio/SFX/SFX_Validate.SFX_Validate");
	constexpr const TCHAR* WallHit = TEXT("/Game/Audio/SFX/SFX_WallHit.SFX_WallHit");
	constexpr const TCHAR* WallBreached = TEXT("/Game/Audio/SFX/SFX_WallBreached.SFX_WallBreached");
	constexpr const TCHAR* WallRepaired = TEXT("/Game/Audio/SFX/SFX_WallRepaired.SFX_WallRepaired");
	constexpr const TCHAR* StationAttended = TEXT("/Game/Audio/SFX/SFX_StationAttended.SFX_StationAttended");
	constexpr const TCHAR* StationWarning = TEXT("/Game/Audio/SFX/SFX_StationWarning.SFX_StationWarning");
	constexpr const TCHAR* StationFailed = TEXT("/Game/Audio/SFX/SFX_StationFailed.SFX_StationFailed");
	constexpr const TCHAR* SupplyDrop = TEXT("/Game/Audio/SFX/SFX_SupplyDrop.SFX_SupplyDrop");
	constexpr const TCHAR* Surge = TEXT("/Game/Audio/SFX/SFX_Surge.SFX_Surge");
	constexpr const TCHAR* LevelWon = TEXT("/Game/Audio/SFX/SFX_LevelWon.SFX_LevelWon");
	constexpr const TCHAR* LevelLost = TEXT("/Game/Audio/SFX/SFX_LevelLost.SFX_LevelLost");
}

UZeroDayAudioComponent::UZeroDayAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

USoundBase* UZeroDayAudioComponent::ResolveSound(const TCHAR* AssetPath)
{
	const FString Path(AssetPath);

	if (TObjectPtr<USoundBase>* Cached = CachedSounds.Find(Path))
	{
		return Cached->Get();
	}

	if (MissingSounds.Contains(Path))
	{
		return nullptr;
	}

	if (USoundBase* Loaded = LoadObject<USoundBase>(nullptr, AssetPath))
	{
		CachedSounds.Add(Path, Loaded);
		return Loaded;
	}

	MissingSounds.Add(Path);
	UE_LOG(LogTemp, Warning, TEXT("ZeroDayAudio: sound asset not found: %s"), AssetPath);
	return nullptr;
}

void UZeroDayAudioComponent::Play2D(const TCHAR* AssetPath, float Volume, float Pitch)
{
	if (USoundBase* Sound = ResolveSound(AssetPath))
	{
		UGameplayStatics::PlaySound2D(this, Sound, Volume, Pitch, 0.f, nullptr, nullptr, false);
	}
}

void UZeroDayAudioComponent::PlayAtLocation(const TCHAR* AssetPath, const FVector& Location, float Volume, float Pitch)
{
	if (USoundBase* Sound = ResolveSound(AssetPath))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, Volume, Pitch, 0.f, nullptr, nullptr, nullptr);
	}
}

void UZeroDayAudioComponent::PlayFire(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Fire, Location, 0.55f, 1.f);
}

void UZeroDayAudioComponent::PlayProjectileImpact(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::ProjectileImpact, Location, 0.65f, FMath::FRandRange(0.95f, 1.05f));
}

void UZeroDayAudioComponent::PlayThreatDestroyed(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::ThreatDestroyed, Location, 0.8f, FMath::FRandRange(0.96f, 1.04f));
}

void UZeroDayAudioComponent::PlayPickup(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Pickup, Location, 0.7f, 1.f);
}

void UZeroDayAudioComponent::PlayScan()
{
	Play2D(ZeroDayAudioPaths::Scan, 0.75f, 1.f);
}

void UZeroDayAudioComponent::PlayValidate(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::Validate, Location, 0.75f, 1.f);
}

void UZeroDayAudioComponent::PlayWallHit(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallHit, Location, 0.65f, FMath::FRandRange(0.96f, 1.04f));
}

void UZeroDayAudioComponent::PlayWallBreached(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallBreached, Location, 0.9f, 1.f);
}

void UZeroDayAudioComponent::PlayWallRepaired(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::WallRepaired, Location, 0.8f, 1.f);
}

void UZeroDayAudioComponent::PlayStationAttended(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationAttended, Location, 0.7f, 1.f);
}

void UZeroDayAudioComponent::PlayStationWarning(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationWarning, Location, 0.65f, 1.f);
}

void UZeroDayAudioComponent::PlayStationFailed(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::StationFailed, Location, 0.85f, 1.f);
}

void UZeroDayAudioComponent::PlaySupplyDrop(const FVector& Location)
{
	PlayAtLocation(ZeroDayAudioPaths::SupplyDrop, Location, 0.8f, 1.f);
}

void UZeroDayAudioComponent::PlaySurge()
{
	Play2D(ZeroDayAudioPaths::Surge, 0.9f, 1.f);
}

void UZeroDayAudioComponent::PlayLevelWon()
{
	Play2D(ZeroDayAudioPaths::LevelWon, 1.f, 1.f);
}

void UZeroDayAudioComponent::PlayLevelLost()
{
	Play2D(ZeroDayAudioPaths::LevelLost, 1.f, 1.f);
}
