#pragma once // Only include this header once per compilation unit.

#include "CoreMinimal.h" // Basic Unreal types.
#include "Components/ActorComponent.h" // Base class for reusable pieces of behaviour.
#include "DetectionComponent.generated.h" // Unreal's generated reflection code (must be last include).

// The player's Detect and Validate tools.
// Insta Scan is fast and free but broad: it flags every inbound threat without saying
// what it is. Validate is targeted and costs a resource: it reveals which flagged
// threats are real, how severe they are, and which are false positives.
UCLASS(ClassGroup = (ZeroDay), meta = (BlueprintSpawnableComponent)) // Shows up in the editor's Add Component list.
class ZERODAYLASTLINE_API UDetectionComponent : public UActorComponent // Exported so other files can use it.
{
	GENERATED_BODY() // Unreal boilerplate for reflected classes.

public:
	UDetectionComponent(); // Turns ticking off; it only reacts to key presses.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection") // Editable in the editor and Blueprints.
	float ScanCooldownSeconds = 8.f; // Seconds between Insta Scans.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection") // Editable in the editor and Blueprints.
	int32 ValidateCost = 1; // Intel spent per Validate.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detection") // Editable in the editor and Blueprints.
	float ValidateRange = 700.f; // How close the player must be to a scanned wall to validate it.

	UFUNCTION(BlueprintCallable, Category = "Detection") // Callable from Blueprints.
	void PerformScan(); // Q: flag every inbound threat.

	UFUNCTION(BlueprintCallable, Category = "Detection") // Callable from Blueprints.
	void PerformValidate(); // F: reveal what the nearest scanned wall's threats really are.

	UFUNCTION(BlueprintPure, Category = "Detection") // A read-only getter.
	float GetScanCooldownRemaining() const; // Seconds until the next scan, for the HUD.

private:
	// World time (seconds) at which the next scan is allowed.
	float NextScanTime = 0.f; // Zero means a scan is available immediately.
};
