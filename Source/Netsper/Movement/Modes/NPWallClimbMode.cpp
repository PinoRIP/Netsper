#include "Movement/Modes/NPWallClimbMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "Movement/NPMoverTypes.h"
#include "Movement/LayeredMoves/NPJumpLayeredMove.h"
#include "Netsper.h"

void UNPWallClimbMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPWallClimbMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPWallClimbMode::Activate()
{
	Super::Activate();
	CachedWallNormal = FVector::ZeroVector;
}

void UNPWallClimbMode::Deactivate()
{
	Super::Deactivate();
}

void UNPWallClimbMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	// During wall climb, we move straight up
	OutProposedMove.LinearVelocity = FVector(0.f, 0.f, ClimbSpeed);
	OutProposedMove.MixMode = EMoveMixMode::OverrideVelocity;
	OutProposedMove.bHasDirIntent = true;
	OutProposedMove.DirectionIntent = FVector::UpVector;
}

void UNPWallClimbMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
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

	OutNPState.ModeElapsedTime += DeltaSeconds;

	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FVector ForwardDir = UpdatedComponent->GetForwardVector();

	// Wall trace forward
	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredComponent(Cast<UPrimitiveComponent>(UpdatedComponent));

	bool bWallFound = GetWorld()->SweepSingleByChannel(
		WallHit, Location, Location + ForwardDir * WallTraceDistance,
		FQuat::Identity, ECC_WorldStatic,
		FCollisionShape::MakeSphere(5.f), QueryParams);

	if (bWallFound)
	{
		CachedWallNormal = WallHit.Normal;
	}

	// If wall not found (reached top), exit with upward boost
	if (!bWallFound)
	{
		FVector ExitVelocity = -CachedWallNormal * 100.f; // Small push away
		ExitVelocity.Z = TopClearanceBoost;

		OutNPState.ModeElapsedTime = 0.f;
		OutSync.SetTransforms_WorldSpace(
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentRotation(),
			ExitVelocity,
			FVector::ZeroVector);

		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Move upward
	const FVector ClimbVelocity = FVector(0.f, 0.f, ClimbSpeed);
	const FVector MoveDelta = ClimbVelocity * DeltaSeconds;
	FHitResult Hit(1.f);
	FMovementRecord MoveRecord;

	UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

	OutSync.SetTransforms_WorldSpace(
		UpdatedComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentRotation(),
		ClimbVelocity,
		FVector::ZeroVector);

	// Jump from wall climb
	const bool bJumpRequested = CharInputs ? CharInputs->bIsJumpJustPressed : false;
	if (bJumpRequested)
	{
		FVector JumpDir = -CachedWallNormal;
		JumpDir.Normalize();

		FVector WallJumpVelocity = JumpDir * MiniJumpForwardVelocity;
		WallJumpVelocity.Z = MiniJumpUpVelocity;

		TSharedPtr<FNPJumpLayeredMove> JumpMove = MakeShared<FNPJumpLayeredMove>();
		JumpMove->JumpVelocity = WallJumpVelocity;
		JumpMove->JumpType = ENPJumpType::WallJump;
		MoverComp->QueueLayeredMove(JumpMove);

		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}

	// Duration exceeded
	if (OutNPState.ModeElapsedTime >= WallClimbMaxDuration)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
		OutputState.MovementEndState.RemainingMs = 0.f;
		return;
	}
}
