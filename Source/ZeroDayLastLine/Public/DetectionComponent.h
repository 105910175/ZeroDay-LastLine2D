#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DetectionComponent.generated.h"

// The player's Detect and Validate tools.
// Insta Scan is fast and free but broad: it flags every inbound threat without saying
// what it is. Validate is targeted and costs a resource: it reveals which flagged
// threats are real, how severe they are, and which are false positives.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent))
class ZERODAYLASTLINE_API UDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDetectionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float ScanCooldownSeconds = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	int32 ValidateCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection")
	float ValidateRange = 700.f;

	UFUNCTION(BlueprintCallable, Category = "Detection")
	void PerformScan();

	UFUNCTION(BlueprintCallable, Category = "Detection")
	void PerformValidate();

	UFUNCTION(BlueprintPure, Category = "Detection")
	float GetScanCooldownRemaining() const;

private:
	// World time (seconds) at which the next scan is allowed.
	float NextScanTime = 0.f;
};
