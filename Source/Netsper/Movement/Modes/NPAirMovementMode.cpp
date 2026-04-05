#include "Movement/Modes/NPAirMovementMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/MoverBlackboard.h"
#include "Movement/NPMoverTypes.h"
#include "Components/CapsuleComponent.h"
#include "Netsper.h"

void UNPAirMovementMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPAirMovementMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPAirMovementMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FCharacterDefaultInputs* CharInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	const FNPMoverState* NPState = StartState.SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();

	if (!SyncState)
	{
		return;
	}

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;

	// Air control: apply partial input
	FVector AirInput = FVector::ZeroVector;
	if (CharInputs)
	{
		AirInput = CharInputs->GetMoveInput() * MaxAirSpeed * AirControlMultiplier;
	}

	OutProposedMove.LinearVelocity = AirInput;
	OutProposedMove.bHasDirIntent = !AirInput.IsNearlyZero();
	OutProposedMove.DirectionIntent = AirInput.GetSafeNormal();
	OutProposedMove.MixMode = EMoveMixMode::AdditiveVelocity;
}

void UNPAirMovementMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	UMoverComponent* MoverComp = GetMoverComponent();
	if (!MoverComp)
	{
		return;
	}

	USceneComponent* UpdatedComponent = Params.MovingComps.UpdatedComponent.Get();
	UMoverBlackboard* SimBlackboard = Params.SimBlackboard;
	const float DeltaSeconds = Params.TimeStep.StepMs * 0.001f;

	if (!UpdatedComponent || DeltaSeconds <= 0.f)
	{
		return;
	}

	// Read input/state
	const FCharacterDefaultInputs* CharInputs = Params.StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
	const FNPMoverInputCmd* NPInput = Params.StartState.InputCmd.InputCollection.FindDataByType<FNPMoverInputCmd>();
	const FMoverDefaultSyncState* StartSync = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	const FNPMoverState* StartNPState = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();

	if (!StartSync)
	{
		return;
	}

	// Output states
	FMoverDefaultSyncState& OutSync = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
	FNPMoverState& OutNPState = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FNPMoverState>();

	if (StartNPState)
	{
		OutNPState = *StartNPState;
	}

	OutNPState.ModeElapsedTime += DeltaSeconds;

	FVector CurrentVelocity = StartSync->GetVelocity_WorldSpace();

	// Handle jump request (coyote time or first frame)
	const bool bJumpRequested = CharInputs ? CharInputs->bIsJumpJustPressed : false;
	const bool bJumpHeld = CharInputs ? CharInputs->bIsJumpPressed : false;

	if (bJumpRequested && (OutNPState.CoyoteTimeRemaining > 0.f || OutNPState.ModeElapsedTime <= DeltaSeconds))
	{
		// Apply jump impulse
		CurrentVelocity.Z = JumpInitialVelocity;
		OutNPState.bIsJumping = true;
		OutNPState.JumpHoldTimeRemaining = JumpHoldMaxDuration;
		OutNPState.CoyoteTimeRemaining = 0.f;
	}

	// Coyote time countdown
	if (OutNPState.CoyoteTimeRemaining > 0.f)
	{
		OutNPState.CoyoteTimeRemaining = FMath::Max(0.f, OutNPState.CoyoteTimeRemaining - DeltaSeconds);
	}

	// Variable jump height: reduced gravity while holding jump
	float EffectiveGravityScale = GravityScale;
	if (OutNPState.bIsJumping && bJumpHeld && OutNPState.JumpHoldTimeRemaining > 0.f && CurrentVelocity.Z > 0.f)
	{
		EffectiveGravityScale = JumpHoldGravityScale;
		OutNPState.JumpHoldTimeRemaining -= DeltaSeconds;
	}
	else
	{
		OutNPState.bIsJumping = false;
		OutNPState.JumpHoldTimeRemaining = 0.f;
	}

	// Apply gravity
	const float GravityZ = GetWorld()->GetGravityZ() * EffectiveGravityScale;
	CurrentVelocity.Z += GravityZ * DeltaSeconds;
	CurrentVelocity.Z = FMath::Max(CurrentVelocity.Z, TerminalVelocity);

	// Air control: blend horizontal velocity toward input direction
	FVector AirControlVelocity = Params.ProposedMove.LinearVelocity;
	FVector HorizontalVelocity(CurrentVelocity.X, CurrentVelocity.Y, 0.f);

	if (!AirControlVelocity.IsNearlyZero())
	{
		HorizontalVelocity = FMath::VInterpConstantTo(HorizontalVelocity, HorizontalVelocity + AirControlVelocity, DeltaSeconds, AirAcceleration);
	}

	// Clamp horizontal speed
	const float HorizSpeed = HorizontalVelocity.Size2D();
	if (HorizSpeed > MaxAirSpeed)
	{
		HorizontalVelocity = HorizontalVelocity.GetSafeNormal2D() * MaxAirSpeed;
	}

	FVector NewVelocity(HorizontalVelocity.X, HorizontalVelocity.Y, CurrentVelocity.Z);

	// Move
	const FVector MoveDelta = NewVelocity * DeltaSeconds;
	FHitResult Hit(1.f);
	FMovementRecord MoveRecord;

	UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

	if (Hit.bBlockingHit)
	{
		// Slide along surface
		UMovementUtils::TryMoveToSlideAlongSurface(Params.MovingComps, MoveDelta, 1.f - Hit.Time, UpdatedComponent->GetComponentQuat(), Hit.Normal, Hit, true, MoveRecord);

		// If we hit something below us and it's walkable, we've landed
		if (Hit.bBlockingHit && Hit.Normal.Z > MaxWalkSlopeCosine)
		{
			const float ImpactSpeed = FMath::Abs(CurrentVelocity.Z);

			// Apply stagger if impact was hard
			if (ImpactSpeed > StaggerThreshold)
			{
				OutNPState.StaggerTimeRemaining = StaggerDuration;
			}

			// Reset air state
			OutNPState.bAirDodgeUsed = false;
			OutNPState.bIsJumping = false;
			OutNPState.JumpHoldTimeRemaining = 0.f;
			OutNPState.CoyoteTimeRemaining = 0.f;
			OutNPState.ModeElapsedTime = 0.f;

			NewVelocity.Z = 0.f;

			OutSync.SetTransforms_WorldSpace(
				UpdatedComponent->GetComponentLocation(),
				UpdatedComponent->GetComponentRotation(),
				NewVelocity,
				FVector::ZeroVector);

			OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
			OutputState.MovementEndState.RemainingMs = Params.TimeStep.StepMs * (1.f - Hit.Time);
			return;
		}
	}

	// Floor check for landing (downward trace)
	FFloorCheckResult FloorResult;
	UFloorQueryUtils::FindFloor(Params.MovingComps, FloorSweepDistance, MaxWalkSlopeCosine, true, UpdatedComponent->GetComponentLocation(), FloorResult);

	if (SimBlackboard)
	{
		SimBlackboard->Set(CommonBlackboard::LastFloorResult, FloorResult);
	}

	if (FloorResult.IsWalkableFloor() && CurrentVelocity.Z <= 0.f && FloorResult.GetDistanceToFloor() < LandingTraceDistance)
	{
		const float ImpactSpeed = FMath::Abs(CurrentVelocity.Z);
		if (ImpactSpeed > StaggerThreshold)
		{
			OutNPState.StaggerTimeRemaining = StaggerDuration;
		}

		OutNPState.bAirDodgeUsed = false;
		OutNPState.bIsJumping = false;
		OutNPState.JumpHoldTimeRemaining = 0.f;
		OutNPState.ModeElapsedTime = 0.f;

		NewVelocity.Z = 0.f;

		OutSync.SetTransforms_WorldSpace(
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentRotation(),
			NewVelocity,
			FVector::ZeroVector);

		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Wall run check
	if (FVector(NewVelocity.X, NewVelocity.Y, 0.f).Size() >= WallRunMinEntrySpeed)
	{
		// Check for walls on left and right
		const FVector Location = UpdatedComponent->GetComponentLocation();
		const FVector ForwardDir = UpdatedComponent->GetForwardVector();
		const FVector RightDir = UpdatedComponent->GetRightVector();

		FHitResult WallHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredComponent(Cast<UPrimitiveComponent>(UpdatedComponent));

		// Right wall check
		bool bFoundWall = GetWorld()->SweepSingleByChannel(
			WallHit, Location, Location + RightDir * WallTraceDistance,
			FQuat::Identity, ECC_WorldStatic,
			FCollisionShape::MakeSphere(5.f), QueryParams);

		if (!bFoundWall)
		{
			// Left wall check
			bFoundWall = GetWorld()->SweepSingleByChannel(
				WallHit, Location, Location - RightDir * WallTraceDistance,
				FQuat::Identity, ECC_WorldStatic,
				FCollisionShape::MakeSphere(5.f), QueryParams);
		}

		if (bFoundWall && FMath::Abs(WallHit.Normal.Z) < 0.3f)
		{
			// Valid wall for wall run
			OutSync.SetTransforms_WorldSpace(
				UpdatedComponent->GetComponentLocation(),
				UpdatedComponent->GetComponentRotation(),
				NewVelocity,
				FVector::ZeroVector);

			OutputState.MovementEndState.NextModeName = NPMovementModeNames::WallRun;
			OutputState.MovementEndState.RemainingMs = 0.f;
			return;
		}
	}

	// Write output
	OutSync.SetTransforms_WorldSpace(
		UpdatedComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentRotation(),
		NewVelocity,
		FVector::ZeroVector);
}
