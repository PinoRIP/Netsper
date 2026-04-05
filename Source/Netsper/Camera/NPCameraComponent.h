#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPCameraComponent.generated.h"

class APlayerCameraManager;
class UMoverComponent;
class UMoverBlackboard;

/**
 * UNPCameraComponent — Manages camera effects (FOV, tilt, shake) via camera modifiers.
 * Reads movement state and applies additive effects. Never sets the camera transform directly.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPCameraComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// FOV
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float BaseFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float MaxFOV = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float MaxSprintSpeed = 950.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|FOV")
	float FOVInterpSpeed = 8.f;

	// Wall run tilt
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Tilt")
	float TiltInterpSpeed = 8.f;

	// Landing shake
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
	float LandingShakeDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
	float LandingShakeIntensity = 3.f;

private:
	float CurrentFOV = 90.f;
	float CurrentTiltRoll = 0.f;
	float TargetTiltRoll = 0.f;
	float LandingShakeTimer = 0.f;
	float LandingShakeAmount = 0.f;
};
