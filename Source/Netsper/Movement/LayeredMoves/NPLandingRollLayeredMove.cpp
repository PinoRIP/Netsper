#include "Movement/LayeredMoves/NPLandingRollLayeredMove.h"
#include "MoverDataModelTypes.h"
#include "MoverComponent.h"

FNPLandingRollLayeredMove::FNPLandingRollLayeredMove()
{
	DurationMs = 350.f; // Roll animation length
	MixMode = EMoveMixMode::OverrideVelocity;
}

bool FNPLandingRollLayeredMove::GenerateMove(const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	if (RollVelocity.IsNearlyZero())
	{
		return false;
	}

	// Decay velocity over the roll duration
	float RemainingPct = 1.f;
	if (DurationMs > 0.f && StartSimTimeMs >= 0.0)
	{
		const double Elapsed = TimeStep.BaseSimTimeMs - StartSimTimeMs;
		RemainingPct = FMath::Clamp(1.f - static_cast<float>(Elapsed / DurationMs), 0.f, 1.f);
	}

	OutProposedMove.LinearVelocity = RollVelocity * RemainingPct;
	OutProposedMove.MixMode = MixMode;
	OutProposedMove.DirectionIntent = RollVelocity.GetSafeNormal();
	OutProposedMove.bHasDirIntent = true;

	return true;
}

FLayeredMoveBase* FNPLandingRollLayeredMove::Clone() const
{
	return new FNPLandingRollLayeredMove(*this);
}

void FNPLandingRollLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);
	Ar << RollVelocity;
}

UScriptStruct* FNPLandingRollLayeredMove::GetScriptStruct() const
{
	return FNPLandingRollLayeredMove::StaticStruct();
}

FString FNPLandingRollLayeredMove::ToSimpleString() const
{
	return FString::Printf(TEXT("NPLandingRoll: Vel=%s"), *RollVelocity.ToString());
}
