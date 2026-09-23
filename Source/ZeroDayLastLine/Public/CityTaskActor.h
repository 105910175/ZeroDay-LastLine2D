#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZeroDayTypes.h"
#include "CityTaskActor.generated.h"

class UCityTaskComponent;
class UCooldownComponent;
class UStaticMeshComponent;
class ACityTaskActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityTaskFailed, ACityTaskActor*, FailedTask);

// Power / food / medic tasks inside the walls. This is the other half of
// Validate & Prioritise: the player has to choose between running to a wall
// or attending a task before its grace period (GraceTimer) runs out.
UCLASS()
class ZERODAYLASTLINE_API ACityTaskActor : public AActor
{
	GENERATED_BODY()

public:
	ACityTaskActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	ECityTaskKind TaskKind = ECityTaskKind::Power;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float GracePeriodSeconds = 8.f;

	UPROPERTY(BlueprintReadOnly, Category = "CityTask")
	bool bIsActiveThisLevel = true;

	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnCityTaskFailed OnTaskFailed;

	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void Attend();

	UFUNCTION(BlueprintPure, Category = "CityTask")
	float GetCondition() const;

	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void SetActiveThisLevel(bool bActive);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UStaticMeshComponent> TaskMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCityTaskComponent> TaskComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCooldownComponent> GraceTimer;

private:
	UFUNCTION()
	void HandleBecameCritical();

	UFUNCTION()
	void HandleAttended();

	UFUNCTION()
	void HandleGraceExpired();

	bool bFailed = false;
};
