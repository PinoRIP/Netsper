#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPStaminaInterface.generated.h"

UINTERFACE(MinimalAPI)
class UNPStaminaProvider : public UInterface
{
	GENERATED_BODY()
};

/** Interface for querying and consuming stamina (SP) from movement modes and abilities */
class NETSPER_API INPStaminaProvider
{
	GENERATED_BODY()

public:
	virtual float GetCurrentSP() const = 0;
	virtual float GetMaxSP() const = 0;

	/** Returns true if sufficient SP available; deducts if so */
	virtual bool TryConsumeSP(float Amount) = 0;

	/** Forced consumption (for predicted deductions from movement modes) */
	virtual void ConsumeSP(float Amount) = 0;

	/** SP gain (pickups, ability effects) */
	virtual void RestoreSP(float Amount) = 0;
};
