#include "Movement/Modes/NPGroundMovementMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/GroundMovementUtils.h"
#include "MoveLibrary/MoverBlackboard.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Movement/NPMoverTypes.h"
#include "Components/CapsuleComponent.h"
#include "Netsper.h"

void UNPGroundMovementMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPGroundMovementMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPGroundMovementMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FCharacterDefaultInputs* CharInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	const FNPMoverInputCmd* NPInput = StartState.InputCmd.InputCollection.FindDataByType<FNPMoverInputCmd>();
	const FNPMoverState* NPState = StartState.SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();

	if (!SyncState)
	{
		return;
	}

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;
	const float TargetSpeed = GetTargetSpeed(NPInput, NPState);
	const float Accel = GetAcceleration(NPInput);

	FVector MoveInput = FVector::ZeroVector;
	FRotator IntentRotation = FRotator::ZeroRotator;

	if (CharInputs)
	{
		MoveInput = CharInputs->GetMoveInput();
		IntentRotation = CharInputs->ControlRotation;
	}

	// Compute desired velocity
	FVector DesiredVelocity = MoveInput * TargetSpeed;

	// Apply stagger penalty
	if (NPState && NPState->StaggerTimeRemaining > 0.f)
	{
		DesiredVelocity *= 0.3f;
	}

	OutProposedMove.LinearVelocity = DesiredVelocity;
	OutProposedMove.DirectionIntent = MoveInput.GetSafeNormal();
	OutProposedMove.bHasDirIntent = !MoveInput.IsNearlyZero();
	OutProposedMove.MixMode = EMoveMixMode::OverrideVelocity;
}

void UNPGroundMovementMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	UMoverComponent* MoverComp = GetMoverComponent();
	if (!MoverComp)
	{
		return;
	}

	const FMoverTickStartData& StartState = Params.StartState;
	USceneComponent* UpdatedComponent = Params.MovingComps.UpdatedComponent.Get();
	UMoverBlackboard* SimBlackboard = Params.SimBlackboard;
	const float DeltaSeconds = Params.TimeStep.StepMs * 0.001f;

	if (!UpdatedComponent || DeltaSeconds <= 0.f)
	{
		return;
	}

	// Read input/state
	const FCharacterDefaultInputs* CharInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	const FNPMoverInputCmd* NPInput = StartState.InputCmd.InputCollection.FindDataByType<FNPMoverInputCmd>();
	const FMoverDefaultSyncState* StartSync = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	const FNPMoverState* StartNPState = StartState.SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();

	if (!StartSync)
	{
		return;
	}

	// Proposed move from GenerateMove
	FProposedMove ProposedMove = Params.ProposedMove;

	// Output states
	FMoverDefaultSyncState& OutSync = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
	FNPMoverState& OutNPState = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FNPMoverState>();

	// Copy starting NP state
	if (StartNPState)
	{
		OutNPState = *StartNPState;
	}

	// Tick predicted SP (ability cost + regen)
	NPStaminaUtils::TickSPFromComponent(OutNPState, UpdatedComponent, DeltaSeconds);

	// Determine sub-state
	const bool bWantsSprint = NPInput ? NPInput->bWantsSprint : false;
	const bool bWantsCrouch = NPInput ? NPInput->bWantsCrouch : false;
	const bool bHasStamina = OutNPState.CurrentSP > 0.f;

	// Update crouch state
	OutNPState.bIsCrouching = bWantsCrouch;
	OutNPState.CapsuleHalfHeight = bWantsCrouch ? CrouchHalfHeight : StandingHalfHeight;

	// Consume SP for sprinting (only if not crouching)
	if (bWantsSprint && !bWantsCrouch && bHasStamina)
	{
		OutNPState.CurrentSP = FMath::Max(0.f, OutNPState.CurrentSP - SprintSPCostPerSecond * DeltaSeconds);
		NPStaminaUtils::NotifyConsumption(OutNPState);
	}

	// Update stagger
	if (OutNPState.StaggerTimeRemaining > 0.f)
	{
		OutNPState.StaggerTimeRemaining = FMath::Max(0.f, OutNPState.StaggerTimeRemaining - DeltaSeconds);
	}

	// Increment mode time
	OutNPState.ModeElapsedTime += DeltaSeconds;

	// Compute velocity using acceleration towards desired
	FVector CurrentVelocity = StartSync->GetVelocity_WorldSpace();
	FVector DesiredVelocity = ProposedMove.LinearVelocity;

	const float TargetSpeed = GetTargetSpeed(NPInput, &OutNPState);
	const float Accel = GetAcceleration(NPInput);

	FVector NewVelocity;
	if (!DesiredVelocity.IsNearlyZero())
	{
		// Accelerate toward desired
		NewVelocity = FMath::VInterpConstantTo(CurrentVelocity, DesiredVelocity, DeltaSeconds, Accel);
	}
	else
	{
		// Brake
		const float CurrentSpeed = CurrentVelocity.Size();
		const float NewSpeed = FMath::Max(0.f, CurrentSpeed - BrakingDeceleration * DeltaSeconds);
		NewVelocity = CurrentSpeed > KINDA_SMALL_NUMBER ? CurrentVelocity.GetSafeNormal() * NewSpeed : FVector::ZeroVector;
	}

	// Apply ground friction
	{
		const float Speed = NewVelocity.Size();
		if (Speed > 0.f)
		{
			const float FrictionForce = GroundFriction * DeltaSeconds;
			const float ClampedFriction = FMath::Min(FrictionForce, 1.f);
			NewVelocity *= (1.f - ClampedFriction);
			// Re-accelerate toward desired to counteract friction when we have input
			if (!DesiredVelocity.IsNearlyZero())
			{
				NewVelocity = FMath::VInterpConstantTo(NewVelocity, DesiredVelocity, DeltaSeconds, Accel * 0.5f);
			}
		}
	}

	// Clamp to target speed
	NewVelocity = NewVelocity.GetClampedToMaxSize(TargetSpeed);
	NewVelocity.Z = 0.f; // Ground mode: no vertical velocity

	// Move
	const FVector MoveDelta = NewVelocity * DeltaSeconds;
	FHitResult Hit(1.f);
	FMovementRecord MoveRecord;

	UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

	if (Hit.bBlockingHit)
	{
		UMovementUtils::TryMoveToSlideAlongSurface(Params.MovingComps, MoveDelta, 1.f - Hit.Time, UpdatedComponent->GetComponentQuat(), Hit.Normal, Hit, true, MoveRecord);
	}

	// Floor check
	FFloorCheckResult FloorResult;
	UFloorQueryUtils::FindFloor(Params.MovingComps, FloorSweepDistance, MaxWalkSlopeCosine, true, UpdatedComponent->GetComponentLocation(), FloorResult);

	if (SimBlackboard)
	{
		SimBlackboard->Set(CommonBlackboard::LastFloorResult, FloorResult);
	}

	// Write output sync state
	OutSync.SetTransforms_WorldSpace(
		UpdatedComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentRotation(),
		NewVelocity,
		FVector::ZeroVector);

	// Mode transitions
	const bool bJumpRequested = CharInputs ? CharInputs->bIsJumpJustPressed : false;
	const float HorizontalSpeed = FVector(NewVelocity.X, NewVelocity.Y, 0.f).Size();

	// Jump → Air mode
	if (bJumpRequested)
	{
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		OutNPState.CoyoteTimeRemaining = 0.f;
		return;
	}

	// Not on walkable floor → Air mode
	if (!FloorResult.IsWalkableFloor())
	{
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = Params.TimeStep.StepMs;
		OutNPState.CoyoteTimeRemaining = 0.15f; // Coyote time
		return;
	}

	// Slide entry: sprinting + crouching + fast enough
	if (bWantsCrouch && HorizontalSpeed >= SlideEntryThreshold)
	{
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Slide;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}
}

float UNPGroundMovementMode::GetTargetSpeed(const FNPMoverInputCmd* NPInput, const FNPMoverState* NPState) const
{
	if (!NPInput)
	{
		return MaxWalkSpeed;
	}

	if (NPInput->bWantsCrouch)
	{
		return MaxCrouchSpeed;
	}

	if (NPInput->bWantsSprint && NPState && NPState->CurrentSP > 0.f)
	{
		return MaxSprintSpeed;
	}

	return MaxWalkSpeed;
}

float UNPGroundMovementMode::GetAcceleration(const FNPMoverInputCmd* NPInput) const
{
	if (NPInput && NPInput->bWantsSprint)
	{
		return SprintAcceleration;
	}
	return WalkAcceleration;
}
