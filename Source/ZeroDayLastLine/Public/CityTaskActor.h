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

	// Which task this station represents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	ECityTaskKind TaskKind = ECityTaskKind::Power;

	// Seconds the player has to attend the task once it turns critical.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CityTask")
	float GracePeriodSeconds = 8.f;

	// Set by the game mode: tasks only run on level 3.
	UPROPERTY(BlueprintReadOnly, Category = "CityTask")
	bool bIsActiveThisLevel = true;

	// Fired if the grace period runs out while the task is still critical.
	UPROPERTY(BlueprintAssignable, Category = "CityTask")
	FOnCityTaskFailed OnTaskFailed;

	// Spends one resource and resets the task, if the player can afford it.
	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void Attend();

	UFUNCTION(BlueprintPure, Category = "CityTask")
	float GetCondition() const;

	// Shows/hides the station and pauses its decay for the current level.
	UFUNCTION(BlueprintCallable, Category = "CityTask")
	void SetActiveThisLevel(bool bActive);

protected:
	virtual void BeginPlay() override;

	// Visible, solid station the player walks up to.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UStaticMeshComponent> TaskMesh;

	// The decaying condition meter.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCityTaskComponent> TaskComp;

	// Countdown started when the task turns critical.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CityTask")
	TObjectPtr<UCooldownComponent> GraceTimer;

private:
	UFUNCTION()
	void HandleBecameCritical();

	UFUNCTION()
	void HandleAttended();

	UFUNCTION()
	void HandleGraceExpired();

	// Latched once failed so the failure only fires once.
	bool bFailed = false;
};
