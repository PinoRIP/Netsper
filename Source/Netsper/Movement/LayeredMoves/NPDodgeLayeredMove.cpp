#include "Movement/LayeredMoves/NPDodgeLayeredMove.h"
#include "MoverDataModelTypes.h"
#include "MoverComponent.h"

FNPDodgeLayeredMove::FNPDodgeLayeredMove()
{
	DurationMs = 200.f;
	MixMode = EMoveMixMode::OverrideVelocity;
}

bool FNPDodgeLayeredMove::GenerateMove(const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	if (DodgeDirection.IsNearlyZero())
	{
		return false;
	}

	// Pure horizontal dodge
	FVector DodgeVelocity = DodgeDirection * DodgeSpeed;
	DodgeVelocity.Z = 0.f;

	OutProposedMove.LinearVelocity = DodgeVelocity;
	OutProposedMove.MixMode = MixMode;
	OutProposedMove.DirectionIntent = DodgeDirection;
	OutProposedMove.bHasDirIntent = true;

	return true;
}

FLayeredMoveBase* FNPDodgeLayeredMove::Clone() const
{
	return new FNPDodgeLayeredMove(*this);
}

void FNPDodgeLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);
	Ar << DodgeDirection;
	Ar << DodgeSpeed;
}

UScriptStruct* FNPDodgeLayeredMove::GetScriptStruct() const
{
	return FNPDodgeLayeredMove::StaticStruct();
}

FString FNPDodgeLayeredMove::ToSimpleString() const
{
	return FString::Printf(TEXT("NPDodge: Dir=%s Speed=%.0f"), *DodgeDirection.ToString(), DodgeSpeed);
}
