#include "Movement/Modes/NPSlideMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/MoverBlackboard.h"
#include "Movement/NPMoverTypes.h"

void UNPSlideMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPSlideMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPSlideMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	if (!SyncState)
	{
		return;
	}

	// Slide preserves current velocity direction with friction
	FVector Velocity = SyncState->GetVelocity_WorldSpace();
	Velocity.Z = 0.f;

	OutProposedMove.LinearVelocity = Velocity;
	OutProposedMove.DirectionIntent = Velocity.GetSafeNormal();
	OutProposedMove.bHasDirIntent = !Velocity.IsNearlyZero();
	OutProposedMove.MixMode = EMoveMixMode::OverrideVelocity;
}

void UNPSlideMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
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

	const FNPMoverInputCmd* NPInput = Params.StartState.InputCmd.InputCollection.FindDataByType<FNPMoverInputCmd>();
	const FMoverDefaultSyncState* StartSync = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	const FNPMoverState* StartNPState = Params.StartState.SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();

	if (!StartSync)
	{
		return;
	}

	FMoverDefaultSyncState& OutSync = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>();
	FNPMoverState& OutNPState = OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FNPMoverState>();

	if (StartNPState)
	{
		OutNPState = *StartNPState;
	}

	// Tick predicted SP (ability cost + regen)
	NPStaminaUtils::TickSPFromComponent(OutNPState, UpdatedComponent, DeltaSeconds);

	OutNPState.ModeElapsedTime += DeltaSeconds;
	OutNPState.bIsCrouching = true;
	OutNPState.CapsuleHalfHeight = CrouchHalfHeight;

	FVector CurrentVelocity = StartSync->GetVelocity_WorldSpace();
	FVector HorizontalVelocity(CurrentVelocity.X, CurrentVelocity.Y, 0.f);

	// Floor check for ramp detection
	FFloorCheckResult FloorResult;
	UFloorQueryUtils::FindFloor(Params.MovingComps, FloorSweepDistance, MaxWalkSlopeCosine, true, UpdatedComponent->GetComponentLocation(), FloorResult);

	if (SimBlackboard)
	{
		SimBlackboard->Set(CommonBlackboard::LastFloorResult, FloorResult);
	}

	// Ramp-based friction adjustment
	float EffectiveFriction = SlideFriction;
	if (FloorResult.IsWalkableFloor())
	{
		const FVector FloorNormal = FloorResult.HitResult.Normal;
		const FVector SlideDir = HorizontalVelocity.GetSafeNormal();
		const float SlopeDot = FVector::DotProduct(FloorNormal, SlideDir);

		if (SlopeDot > 0.05f)
		{
			// Downhill: reduce friction, boost
			EffectiveFriction *= 0.2f;
			HorizontalVelocity += SlideDir * DownhillBoostScale * SlopeDot * DeltaSeconds * 100.f;
		}
		else if (SlopeDot < -0.05f)
		{
			// Uphill: increase friction
			EffectiveFriction *= UphillFrictionScale;
		}
	}

	// Apply friction
	const float Speed = HorizontalVelocity.Size();
	if (Speed > 0.f)
	{
		const float FrictionDecel = EffectiveFriction * Speed * DeltaSeconds;
		const float NewSpeed = FMath::Max(0.f, Speed - FrictionDecel);
		HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * NewSpeed;
	}

	FVector NewVelocity(HorizontalVelocity.X, HorizontalVelocity.Y, 0.f);

	// Move
	const FVector MoveDelta = NewVelocity * DeltaSeconds;
	FHitResult Hit(1.f);
	FMovementRecord MoveRecord;

	UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

	if (Hit.bBlockingHit)
	{
		UMovementUtils::TryMoveToSlideAlongSurface(Params.MovingComps, MoveDelta, 1.f - Hit.Time, UpdatedComponent->GetComponentQuat(), Hit.Normal, Hit, true, MoveRecord);
	}

	OutSync.SetTransforms_WorldSpace(
		UpdatedComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentRotation(),
		NewVelocity,
		FVector::ZeroVector);

	// Exit conditions
	const float HorizSpeed = FVector(NewVelocity.X, NewVelocity.Y, 0.f).Size();
	const bool bCrouchHeld = NPInput ? NPInput->bWantsToCrouch : false;

	// Released crouch and slow enough → back to ground
	if (!bCrouchHeld && HorizSpeed < MaxWalkSpeed)
	{
		OutNPState.bIsCrouching = false;
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Speed too low → back to ground
	if (HorizSpeed < SlideExitSpeed)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Left ground → air
	if (!FloorResult.IsWalkableFloor())
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = Params.TimeStep.StepMs;
		return;
	}
}
