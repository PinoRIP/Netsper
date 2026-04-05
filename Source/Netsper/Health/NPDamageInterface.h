#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "NPDamageInterface.generated.h"

UINTERFACE(MinimalAPI)
class UNPDamageable : public UInterface
{
	GENERATED_BODY()
};

/** Interface for applying damage and healing to actors */
class NETSPER_API INPDamageable
{
	GENERATED_BODY()

public:
	virtual void ApplyDamage(float Amount, AActor* Instigator, FGameplayTag DamageType) = 0;
	virtual void ApplyHeal(float Amount) = 0;
	virtual bool IsAlive() const = 0;
};
