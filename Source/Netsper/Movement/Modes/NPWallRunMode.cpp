#include "Movement/Modes/NPWallRunMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "MoveLibrary/MoverBlackboard.h"
#include "Movement/NPMoverTypes.h"
#include "Movement/LayeredMoves/NPJumpLayeredMove.h"
#include "Netsper.h"

void UNPWallRunMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPWallRunMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPWallRunMode::Activate()
{
	Super::Activate();
	CachedWallNormal = FVector::ZeroVector;
	bIsLeftWall = false;
}

void UNPWallRunMode::Deactivate()
{
	Super::Deactivate();
}

void UNPWallRunMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	if (!SyncState)
	{
		return;
	}

	FVector Velocity = SyncState->GetVelocity_WorldSpace();
	Velocity.Z = 0.f; // Wall run is mostly horizontal

	OutProposedMove.LinearVelocity = Velocity;
	OutProposedMove.DirectionIntent = Velocity.GetSafeNormal();
	OutProposedMove.bHasDirIntent = !Velocity.IsNearlyZero();
	OutProposedMove.MixMode = EMoveMixMode::OverrideVelocity;
}

void UNPWallRunMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{
	UMoverComponent* MoverComp = GetMoverComponent();
	if (!MoverComp)
	{
		return;
	}

	USceneComponent* UpdatedComponent = Params.MovingComps.UpdatedComponent.Get();
	const float DeltaSeconds = Params.TimeStep.StepMs * 0.001f;

	if (!UpdatedComponent || DeltaSeconds <= 0.f)
	{
		return;
	}

	const FCharacterDefaultInputs* CharInputs = Params.StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();
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

	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FVector RightDir = UpdatedComponent->GetRightVector();

	// Wall trace to confirm wall still present
	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredComponent(Cast<UPrimitiveComponent>(UpdatedComponent));

	bool bWallFound = false;

	// On first tick, detect which side the wall is on
	if (CachedWallNormal.IsNearlyZero())
	{
		// Try right
		bWallFound = GetWorld()->SweepSingleByChannel(
			WallHit, Location, Location + RightDir * WallTraceDistance,
			FQuat::Identity, ECC_WorldStatic,
			FCollisionShape::MakeSphere(5.f), QueryParams);

		if (bWallFound && FMath::Abs(WallHit.Normal.Z) < 0.3f)
		{
			CachedWallNormal = WallHit.Normal;
			bIsLeftWall = false;
		}
		else
		{
			// Try left
			bWallFound = GetWorld()->SweepSingleByChannel(
				WallHit, Location, Location - RightDir * WallTraceDistance,
				FQuat::Identity, ECC_WorldStatic,
				FCollisionShape::MakeSphere(5.f), QueryParams);

			if (bWallFound && FMath::Abs(WallHit.Normal.Z) < 0.3f)
			{
				CachedWallNormal = WallHit.Normal;
				bIsLeftWall = true;
			}
		}
	}
	else
	{
		// Continuous trace in known wall direction
		const FVector TraceDir = bIsLeftWall ? -RightDir : RightDir;
		bWallFound = GetWorld()->SweepSingleByChannel(
			WallHit, Location, Location + TraceDir * WallTraceDistance,
			FQuat::Identity, ECC_WorldStatic,
			FCollisionShape::MakeSphere(5.f), QueryParams);

		if (bWallFound && FMath::Abs(WallHit.Normal.Z) < 0.3f)
		{
			CachedWallNormal = WallHit.Normal;
		}
		else
		{
			bWallFound = false;
		}
	}

	// If wall not found, exit to air
	if (!bWallFound)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutSync.SetTransforms_WorldSpace(
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentRotation(),
			StartSync->GetVelocity_WorldSpace(),
			FVector::ZeroVector);

		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = Params.TimeStep.StepMs;
		return;
	}

	// Compute tangent run direction along wall
	const FVector UpDir = FVector::UpVector;
	FVector WallTangent = FVector::CrossProduct(CachedWallNormal, UpDir);

	// Ensure tangent goes in the direction of player velocity
	const FVector CurrentVelocity = StartSync->GetVelocity_WorldSpace();
	if (FVector::DotProduct(WallTangent, CurrentVelocity) < 0.f)
	{
		WallTangent = -WallTangent;
	}

	// Preserve speed along wall tangent
	const float Speed = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f).Size();
	FVector NewVelocity = WallTangent * Speed;

	// Apply gradual gravity drift
	const float DriftFraction = FMath::Clamp(OutNPState.ModeElapsedTime / WallRunMaxDuration, 0.f, 1.f);
	NewVelocity.Z = -WallRunGravityDrift * DriftFraction;

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

	// Write camera tilt hint to blackboard
	if (Params.SimBlackboard)
	{
		const float TiltTarget = bIsLeftWall ? CameraTiltAngle : -CameraTiltAngle;
		Params.SimBlackboard->Set<float>(FName(TEXT("WallRunTilt")), TiltTarget);
	}

	// Jump from wall
	const bool bJumpRequested = CharInputs ? CharInputs->bIsJumpJustPressed : false;
	if (bJumpRequested)
	{
		FVector WallJumpVelocity = CachedWallNormal * WallJumpAwayVelocity;
		WallJumpVelocity.Z = WallJumpUpVelocity;

		TSharedPtr<FNPJumpLayeredMove> JumpMove = MakeShared<FNPJumpLayeredMove>();
		JumpMove->JumpVelocity = WallJumpVelocity;
		JumpMove->JumpType = ENPJumpType::WallJump;
		MoverComp->QueueLayeredMove(JumpMove);

		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Speed below minimum → exit
	if (Speed < WallRunMinContinueSpeed)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Duration exceeded
	if (OutNPState.ModeElapsedTime >= WallRunMaxDuration)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}
}
