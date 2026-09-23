#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZeroDayTypes.h"
#include "WallActor.generated.h"

class UBreachableComponent;
class UCooldownComponent;
class UStaticMeshComponent;
class UBoxComponent;
class AWallActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallLost, AWallActor*, LostWall);

// One of the four city walls. Detect = Breachable's Threatened/Breached status.
// Remediate = AttemptFix() (patch) or simply surviving the RemediationTimer window
// by killing off the threats attacking it (contain).
UCLASS()
class ZERODAYLASTLINE_API AWallActor : public AActor
{
	GENERATED_BODY()

public:
	AWallActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	EWallSide WallSide = EWallSide::North;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall")
	float RemediationWindowSeconds = 12.f;

	UPROPERTY(BlueprintReadOnly, Category = "Wall")
	bool bIsActiveThisLevel = true;

	UPROPERTY(BlueprintAssignable, Category = "Wall")
	FOnWallLost OnWallLost;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void TakeThreatDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void AttemptFix();

	UFUNCTION(BlueprintPure, Category = "Wall")
	EWallStatus GetStatus() const;

	UFUNCTION(BlueprintCallable, Category = "Wall")
	void SetActiveThisLevel(bool bActive);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBoxComponent> ThreatContactZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UBreachableComponent> Breachable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall")
	TObjectPtr<UCooldownComponent> RemediationTimer;

private:
	UFUNCTION()
	void HandleStatusChanged(EWallStatus NewStatus);

	UFUNCTION()
	void HandleRemediationExpired();

	UFUNCTION()
	void HandleThreatContactBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
