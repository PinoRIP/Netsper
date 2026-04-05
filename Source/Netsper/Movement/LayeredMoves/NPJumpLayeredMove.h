#pragma once

#include "CoreMinimal.h"
#include "LayeredMove.h"
#include "NPJumpLayeredMove.generated.h"

/** Jump variant type */
UENUM(BlueprintType)
enum class ENPJumpType : uint8
{
	Ground    UMETA(DisplayName = "Ground Jump"),
	WallJump  UMETA(DisplayName = "Wall Jump"),
	WallRun   UMETA(DisplayName = "Wall Run Jump"),
};

/**
 * FNPJumpLayeredMove — Applies a one-tick jump impulse.
 * Supports ground, wall-jump, and wall-run jump variants.
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPJumpLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPJumpLayeredMove();
	virtual ~FNPJumpLayeredMove() {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	FVector JumpVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover")
	ENPJumpType JumpType = ENPJumpType::Ground;

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
struct TStructOpsTypeTraits<FNPJumpLayeredMove> : public TStructOpsTypeTraitsBase2<FNPJumpLayeredMove>
{
	enum { WithCopy = true };
};
