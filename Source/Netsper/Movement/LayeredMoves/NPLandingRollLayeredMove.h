#pragma once

#include "CoreMinimal.h"
#include "LayeredMove.h"
#include "NPLandingRollLayeredMove.generated.h"

/**
 * FNPLandingRollLayeredMove — Converts hard-landing velocity into forward momentum.
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPLandingRollLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPLandingRollLayeredMove();
	virtual ~FNPLandingRollLayeredMove() {}

	/** Forward momentum from converted impact speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	FVector RollVelocity = FVector::ZeroVector;

	// FLayeredMoveBase overrides
	virtual bool GenerateMove(const FMoverTickStartData& StartState,
		const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
		UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove) override;
	virtual FLayeredMoveBase* Clone() const override;
	virtual void NetSerialize(FArchive& Ar) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FString ToSimpleString() const override;
};

template<>
struct TStructOpsTypeTraits<FNPLandingRollLayeredMove> : public TStructOpsTypeTraitsBase2<FNPLandingRollLayeredMove>
{
	enum { WithCopy = true };
};
