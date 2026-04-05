#pragma once

#include "CoreMinimal.h"
#include "NPCameraTypes.generated.h"

USTRUCT(BlueprintType)
struct NETSPER_API FNPCameraShakeParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float Intensity = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float Duration = 0.3f;
};
