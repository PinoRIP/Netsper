#include "Movement/LayeredMoves/NPGrappleLayeredMove.h"
#include "MoverDataModelTypes.h"
#include "MoverComponent.h"

FNPGrappleLayeredMove::FNPGrappleLayeredMove()
{
	DurationMs = 0.f; // Infinite — managed by ability
	MixMode = EMoveMixMode::AdditiveVelocity;
}

bool FNPGrappleLayeredMove::GenerateMove(const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	const float DeltaSeconds = TimeStep.StepMs / 1000.f;
	if (DeltaSeconds <= 0.f || HookPoint.IsNearlyZero())
	{
		return false;
	}

	// Get current location from the mover component
	const FVector CurrentLocation = MoverComp->GetUpdatedComponent()->GetComponentLocation();
	const FVector ToHook = HookPoint - CurrentLocation;
	const float DistToHook = ToHook.Size();

	// If close enough, signal completion
	if (DistToHook < DetachDistance)
	{
		return false;
	}

	// Pull toward hook point
	const FVector PullDirection = ToHook.GetSafeNormal();
	FVector PullVelocity = PullDirection * PullForce * DeltaSeconds;

	// Dampen as we approach 
	const float DampingFactor = FMath::Clamp(DistToHook / 500.f, 0.2f, 1.f);
	PullVelocity *= DampingFactor;

	// Clamp final velocity
	if (PullVelocity.SizeSquared() > MaxGrappleSpeed * MaxGrappleSpeed)
	{
		PullVelocity = PullVelocity.GetSafeNormal() * MaxGrappleSpeed;
	}

	OutProposedMove.LinearVelocity = PullVelocity;
	OutProposedMove.MixMode = MixMode;
	OutProposedMove.DirectionIntent = PullDirection;
	OutProposedMove.bHasDirIntent = true;

	return true;
}

FLayeredMoveBase* FNPGrappleLayeredMove::Clone() const
{
	return new FNPGrappleLayeredMove(*this);
}

void FNPGrappleLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);
	Ar << HookPoint;
	Ar << PullForce;
	Ar << DetachDistance;
	Ar << MaxGrappleSpeed;
}

UScriptStruct* FNPGrappleLayeredMove::GetScriptStruct() const
{
	return FNPGrappleLayeredMove::StaticStruct();
}

FString FNPGrappleLayeredMove::ToSimpleString() const
{
	return FString::Printf(TEXT("NPGrapple: Hook=%s Pull=%.0f"), *HookPoint.ToString(), PullForce);
}
