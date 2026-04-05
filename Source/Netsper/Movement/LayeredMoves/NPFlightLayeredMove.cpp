#include "Movement/LayeredMoves/NPFlightLayeredMove.h"
#include "MoverDataModelTypes.h"
#include "MoverComponent.h"

FNPFlightLayeredMove::FNPFlightLayeredMove()
{
	DurationMs = 0.f; // Infinite duration — managed by the ability
	MixMode = EMoveMixMode::OverrideAll;
}

bool FNPFlightLayeredMove::GenerateMove(const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep, const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard, FProposedMove& OutProposedMove)
{
	const float DeltaSeconds = TimeStep.StepMs / 1000.f;
	if (DeltaSeconds <= 0.f)
	{
		return false;
	}

	// Read input: use control rotation for 6DoF direction
	const FCharacterDefaultInputs* DefaultInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	FVector MoveInput = FVector::ZeroVector;

	if (DefaultInputs)
	{
		const FVector InputDir = DefaultInputs->GetMoveInput();
		const FRotator ControlRot = DefaultInputs->ControlRotation;

		// Full 3D movement: use control rotation pitch for up/down
		const FVector Forward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(FRotator(0.f, ControlRot.Yaw, 0.f)).GetUnitAxis(EAxis::Y);

		MoveInput = Forward * InputDir.X + Right * InputDir.Y;
		if (!MoveInput.IsNearlyZero())
		{
			MoveInput.Normalize();
		}
	}

	// Accelerate toward desired direction
	const FVector DesiredVelocity = MoveInput * MaxFlightSpeed;
	FlightVelocity = FMath::VInterpTo(FlightVelocity, DesiredVelocity, DeltaSeconds, FlightAcceleration / MaxFlightSpeed);

	// Clamp to max speed
	if (FlightVelocity.SizeSquared() > MaxFlightSpeed * MaxFlightSpeed)
	{
		FlightVelocity = FlightVelocity.GetSafeNormal() * MaxFlightSpeed;
	}

	OutProposedMove.LinearVelocity = FlightVelocity;
	OutProposedMove.MixMode = MixMode;

	if (!MoveInput.IsNearlyZero())
	{
		OutProposedMove.DirectionIntent = MoveInput;
		OutProposedMove.bHasDirIntent = true;
	}

	return true;
}

FLayeredMoveBase* FNPFlightLayeredMove::Clone() const
{
	return new FNPFlightLayeredMove(*this);
}

void FNPFlightLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);
	Ar << MaxFlightSpeed;
	Ar << FlightAcceleration;
	Ar << FlightVelocity;
}

UScriptStruct* FNPFlightLayeredMove::GetScriptStruct() const
{
	return FNPFlightLayeredMove::StaticStruct();
}

FString FNPFlightLayeredMove::ToSimpleString() const
{
	return FString::Printf(TEXT("NPFlight: Speed=%.0f Vel=%s"), MaxFlightSpeed, *FlightVelocity.ToString());
}
