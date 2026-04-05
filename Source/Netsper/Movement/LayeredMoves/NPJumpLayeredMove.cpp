#include "Movement/LayeredMoves/NPJumpLayeredMove.h"
#include "MoverDataModelTypes.h"
#include "MoverComponent.h"

FNPJumpLayeredMove::FNPJumpLayeredMove()
{
	DurationMs = 0.f; // Single tick (instant)
	MixMode = EMoveMixMode::OverrideVelocity;
}

bool FNPJumpLayeredMove::GenerateMove(const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();

	if (SyncState)
	{
		FVector CurrentVel = SyncState->GetVelocity_WorldSpace();

		switch (JumpType)
		{
		case ENPJumpType::Ground:
			// Preserve horizontal, override vertical
			OutProposedMove.LinearVelocity = FVector(CurrentVel.X, CurrentVel.Y, JumpVelocity.Z);
			break;

		case ENPJumpType::WallJump:
		case ENPJumpType::WallRun:
			// Apply full jump velocity (includes wall-away component)
			OutProposedMove.LinearVelocity = JumpVelocity;
			break;
		}
	}
	else
	{
		OutProposedMove.LinearVelocity = JumpVelocity;
	}

	OutProposedMove.MixMode = MixMode;
	OutProposedMove.PreferredMode = NAME_None;

	return true;
}

FLayeredMoveBase* FNPJumpLayeredMove::Clone() const
{
	return new FNPJumpLayeredMove(*this);
}

void FNPJumpLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);

	bool bHasJumpVelocity = !JumpVelocity.IsNearlyZero();
	Ar.SerializeBits(&bHasJumpVelocity, 1);
	if (bHasJumpVelocity)
	{
		Ar << JumpVelocity;
	}
	else if (Ar.IsLoading())
	{
		JumpVelocity = FVector::ZeroVector;
	}

	uint8 TypeByte = static_cast<uint8>(JumpType);
	Ar << TypeByte;
	if (Ar.IsLoading())
	{
		JumpType = static_cast<ENPJumpType>(TypeByte);
	}
}

UScriptStruct* FNPJumpLayeredMove::GetScriptStruct() const
{
	return FNPJumpLayeredMove::StaticStruct();
}

FString FNPJumpLayeredMove::ToSimpleString() const
{
	return FString::Printf(TEXT("NPJump: Type=%d Vel=%s"), static_cast<int32>(JumpType), *JumpVelocity.ToString());
}
