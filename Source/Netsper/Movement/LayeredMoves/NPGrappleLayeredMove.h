#pragma once

#include "CoreMinimal.h"
#include "LayeredMove.h"
#include "NPGrappleLayeredMove.generated.h"

/**
 * FNPGrappleLayeredMove — Pulls the player toward a grapple hook point.
 *
 * Adds velocity toward the hook point each tick. Does not halt
 * horizontal momentum — adds to it (Apex Legends style).
 */
USTRUCT()
struct NETSPER_API FNPGrappleLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPGrappleLayeredMove();

	/** World-space location of the grapple hook attachment */
	UPROPERTY()
	FVector HookPoint = FVector::ZeroVector;

	/** Pull force magnitude (cm/s²) */
	UPROPERTY()
	float PullForce = 2500.f;

	/** Distance at which to detach (cm) */
	UPROPERTY()
	float DetachDistance = 150.f;

	/** Max speed while grappling */
	UPROPERTY()
	float MaxGrappleSpeed = 1800.f;

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
struct TStructOpsTypeTraits<FNPGrappleLayeredMove> : public TStructOpsTypeTraitsBase2<FNPGrappleLayeredMove>
{
	enum { WithCopy = true };
};
