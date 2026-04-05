#include "Movement/Modes/NPMantleMode.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MovementUtils.h"
#include "Movement/NPMoverTypes.h"
#include "Netsper.h"

void UNPMantleMode::OnRegistered(const FName ModeName)
{
	Super::OnRegistered(ModeName);
}

void UNPMantleMode::OnUnregistered()
{
	Super::OnUnregistered();
}

void UNPMantleMode::Activate()
{
	Super::Activate();
	LedgeTopPosition = FVector::ZeroVector;
	bIsPullingUp = false;
	bIsSPBoosted = false;
}

void UNPMantleMode::Deactivate()
{
	Super::Deactivate();
}

void UNPMantleMode::GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	if (bIsPullingUp)
	{
		const FMoverDefaultSyncState* SyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
		if (SyncState)
		{
			FVector Forward = SyncState->GetOrientation_WorldSpace().Vector();
			Forward.Z = 0.f;
			Forward.Normalize();
			OutProposedMove.LinearVelocity = Forward * MantleHorizontalSpeed;
		}
	}
	else
	{
		OutProposedMove.LinearVelocity = FVector(0.f, 0.f, MantleVerticalSpeed);
	}

	OutProposedMove.MixMode = EMoveMixMode::OverrideVelocity;
	OutProposedMove.bHasDirIntent = true;
	OutProposedMove.DirectionIntent = FVector::UpVector;
}

void UNPMantleMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
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
	OutNPState.bIsCrouching = true; // Stay crouched during mantle
	OutNPState.CapsuleHalfHeight = CrouchHalfHeight;

	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FVector ForwardDir = UpdatedComponent->GetForwardVector();

	// On first tick, detect the ledge
	if (LedgeTopPosition.IsNearlyZero())
	{
		// SP-boost check
		const bool bWantsSprint = NPInput ? NPInput->bWantsSprint : false;
		if (bWantsSprint && OutNPState.CurrentSP >= SPBoostMantleCost)
		{
			// SP-boosted mantle: consume SP and launch upward
			OutNPState.CurrentSP -= SPBoostMantleCost;
			NPStaminaUtils::NotifyConsumption(OutNPState);
			bIsSPBoosted = true;

			FVector LaunchVel = ForwardDir * 200.f;
			LaunchVel.Z = SPBoostUpwardImpulse;

			OutNPState.ModeElapsedTime = 0.f;
			OutNPState.bIsCrouching = false;

			OutSync.SetTransforms_WorldSpace(Location, UpdatedComponent->GetComponentRotation(), LaunchVel, FVector::ZeroVector);

			OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
			OutputState.MovementEndState.RemainingMs = 0.f;
			return;
		}

		// Detect ledge: trace forward at chest height, then trace up from impact
		FHitResult ForwardHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredComponent(Cast<UPrimitiveComponent>(UpdatedComponent));

		const FVector ChestLocation = Location + FVector(0.f, 0.f, 20.f);
		bool bFoundWall = GetWorld()->LineTraceSingleByChannel(
			ForwardHit, ChestLocation, ChestLocation + ForwardDir * LedgeDetectForwardDistance,
			ECC_WorldStatic, QueryParams);

		if (bFoundWall)
		{
			// Trace up from wall impact point
			FHitResult UpHit;
			const FVector WallPoint = ForwardHit.ImpactPoint;
			const FVector TraceStart = WallPoint + FVector(0.f, 0.f, LedgeDetectUpDistance);
			const FVector TraceEnd = WallPoint;

			bool bFoundLedge = GetWorld()->LineTraceSingleByChannel(
				UpHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

			if (bFoundLedge)
			{
				LedgeTopPosition = UpHit.ImpactPoint + FVector(0.f, 0.f, CrouchHalfHeight + 5.f);
			}
			else
			{
				// Open space above — ledge top is at trace start
				LedgeTopPosition = TraceStart;
			}
		}
		else
		{
			// No wall found — abort mantle
			OutNPState.ModeElapsedTime = 0.f;
			OutSync.SetTransforms_WorldSpace(Location, UpdatedComponent->GetComponentRotation(), FVector::ZeroVector, FVector::ZeroVector);
			OutputState.MovementEndState.NextModeName = NPMovementModeNames::Air;
			OutputState.MovementEndState.RemainingMs = 0.f;
			return;
		}
	}

	// Phase 1: Move upward until we clear the ledge
	if (!bIsPullingUp)
	{
		if (Location.Z >= LedgeTopPosition.Z - 5.f)
		{
			bIsPullingUp = true;
		}
		else
		{
			const FVector ClimbVelocity(0.f, 0.f, MantleVerticalSpeed);
			const FVector MoveDelta = ClimbVelocity * DeltaSeconds;
			FHitResult Hit(1.f);
			FMovementRecord MoveRecord;

			UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

			OutSync.SetTransforms_WorldSpace(
				UpdatedComponent->GetComponentLocation(),
				UpdatedComponent->GetComponentRotation(),
				ClimbVelocity,
				FVector::ZeroVector);
		}
	}

	// Phase 2: Move forward onto the ledge
	if (bIsPullingUp)
	{
		FVector PullForward = ForwardDir * MantleHorizontalSpeed;
		PullForward.Z = 0.f;

		const FVector MoveDelta = PullForward * DeltaSeconds;
		FHitResult Hit(1.f);
		FMovementRecord MoveRecord;

		UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, UpdatedComponent->GetComponentQuat(), true, Hit, ETeleportType::None, MoveRecord);

		OutSync.SetTransforms_WorldSpace(
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentRotation(),
			PullForward,
			FVector::ZeroVector);

		// Check if we're now roughly above the ledge
		const float ForwardDist = FVector::Dist2D(Location, LedgeTopPosition);
		if (ForwardDist < 20.f || Hit.bBlockingHit)
		{
			OutNPState.ModeElapsedTime = 0.f;
			OutNPState.bIsCrouching = false;
			OutNPState.CapsuleHalfHeight = 48.f;

			OutSync.SetTransforms_WorldSpace(
				UpdatedComponent->GetComponentLocation(),
				UpdatedComponent->GetComponentRotation(),
				FVector::ZeroVector,
				FVector::ZeroVector);

			OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
			OutputState.MovementEndState.RemainingMs = 0.f;
			return;
		}
	}

	// Duration cap
	if (OutNPState.ModeElapsedTime >= MantleMaxDuration)
	{
		OutNPState.ModeElapsedTime = 0.f;
		OutNPState.bIsCrouching = false;

		OutSync.SetTransforms_WorldSpace(
			UpdatedComponent->GetComponentLocation(),
			UpdatedComponent->GetComponentRotation(),
			FVector::ZeroVector,
			FVector::ZeroVector);

		OutputState.MovementEndState.NextModeName = NPMovementModeNames::Ground;
		OutputState.MovementEndState.RemainingMs = 0.f;
	}
}
