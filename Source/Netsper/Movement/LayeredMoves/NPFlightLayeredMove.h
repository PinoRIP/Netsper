#pragma once

#include "CoreMinimal.h"
#include "LayeredMove.h"
#include "NPFlightLayeredMove.generated.h"

/**
 * FNPFlightLayeredMove — 6DoF flight override layered move.
 *
 * Overrides gravity while active, applies directional flight
 * based on camera orientation and movement input.
 */
USTRUCT()
struct NETSPER_API FNPFlightLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPFlightLayeredMove();

	/** Max flight speed (cm/s) */
	UPROPERTY()
	float MaxFlightSpeed = 900.f;

	/** Flight acceleration rate (cm/s²) */
	UPROPERTY()
	float FlightAcceleration = 1800.f;

	/** Current flight velocity */
	UPROPERTY()
	FVector FlightVelocity = FVector::ZeroVector;

	// FLayeredMoveBase interface
	virtual bool GenerateMove(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep,
	                          const UMoverComponent* MoverComp, UMoverBlackboard* SimBlackboard,
	                          FProposedMove& OutProposedMove) override;
	virtual FLayeredMoveBase* Clone() const override;
	virtual void NetSerialize(FArchive& Ar) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FNPFlightLayeredMove> : public TStructOpsTypeTraitsBase2<FNPFlightLayeredMove>
{
	enum { WithCopy = true };
};
