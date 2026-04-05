#pragma once

#include "CoreMinimal.h"
#include "NPStaminaTypes.generated.h"

USTRUCT(BlueprintType)
struct NETSPER_API FNPStaminaSyncState
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float CurrentSP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float MaxSP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float RegenRate = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float RegenDelay = 1.2f;

	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	float RegenDelayRemaining = 0.f;
};
