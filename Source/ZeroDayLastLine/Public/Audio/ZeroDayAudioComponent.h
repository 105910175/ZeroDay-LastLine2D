#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZeroDayAudioComponent.generated.h"

class USoundBase;

/**
 * Centralised sound-effect player for Zero Day: Last Line.
 *
 * The component loads sounds lazily from /Game/Audio/SFX so the project can still
 * compile and run while audio assets are being prepared. Missing sounds are
 * reported once in the log and otherwise ignored safely.
 */
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent))
class ZERODAYLASTLINE_API UZeroDayAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZeroDayAudioComponent();

	void PlayFire(const FVector& Location);
	void PlayProjectileImpact(const FVector& Location);
	void PlayThreatDestroyed(const FVector& Location);
	void PlayPickup(const FVector& Location);
	void PlayScan();
	void PlayValidate(const FVector& Location);
	void PlayWallHit(const FVector& Location);
	void PlayWallBreached(const FVector& Location);
	void PlayWallRepaired(const FVector& Location);
	void PlayStationAttended(const FVector& Location);
	void PlayStationWarning(const FVector& Location);
	void PlayStationFailed(const FVector& Location);
	void PlaySupplyDrop(const FVector& Location);
	void PlaySurge();
	void PlayLevelWon();
	void PlayLevelLost();

private:
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<USoundBase>> CachedSounds;

	TSet<FString> MissingSounds;

	USoundBase* ResolveSound(const TCHAR* AssetPath);
	void Play2D(const TCHAR* AssetPath, float Volume = 1.f, float Pitch = 1.f);
	void PlayAtLocation(const TCHAR* AssetPath, const FVector& Location, float Volume = 1.f, float Pitch = 1.f);
};
