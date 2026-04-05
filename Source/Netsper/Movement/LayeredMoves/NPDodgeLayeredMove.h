#pragma once

#include "CoreMinimal.h"
#include "LayeredMove.h"
#include "NPDodgeLayeredMove.generated.h"

/**
 * FNPDodgeLayeredMove — Quick directional dash (ground or air).
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPDodgeLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPDodgeLayeredMove();
	virtual ~FNPDodgeLayeredMove() {}

	/** Dodge direction (world space, normalized) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	FVector DodgeDirection = FVector::ZeroVector;

	/** Speed boost during dodge */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	float DodgeSpeed = 1200.f;

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
struct TStructOpsTypeTraits<FNPDodgeLayeredMove> : public TStructOpsTypeTraitsBase2<FNPDodgeLayeredMove>
{
	enum { WithCopy = true };
};
