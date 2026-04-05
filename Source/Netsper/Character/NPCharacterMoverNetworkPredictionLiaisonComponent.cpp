// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCharacterMoverNetworkPredictionLiaisonComponent.h"
#include "Internationalization/Internationalization.h"
#include "MoverComponent.h"
#include "NetworkPredictionWorldManager.h"
#include "NPCharacterNetworkPredictionComponent.h"
#include "NPCharacterNetworkPredictionTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif


// Sets default values for this component's properties
UNPCharacterMoverNetworkPredictionLiaisonComponent::UNPCharacterMoverNetworkPredictionLiaisonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::ProduceInput(const int32 DeltaTimeMS, FMoverInputCmdContext* Cmd)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn))
	{
		return;
	}
	
	FCharacterDefaultInputs& DefaultInputs = Cmd->InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	
	const FRotator ControlRot = OwnerPawn->GetControlRotation();
	const FRotator YawOnlyRot(0.f, ControlRot.Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::Y);

	FVector MoveDirection = ForwardDir * PendingMoveDirection.Y + RightDir * PendingMoveDirection.X;
	if (!MoveDirection.IsNearlyZero())
	{
		MoveDirection.Normalize();
	}

	DefaultInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	DefaultInputs.ControlRotation = ControlRot;
	DefaultInputs.OrientationIntent = ForwardDir;
	DefaultInputs.bIsJumpJustPressed = bJustWantsToJump;
	DefaultInputs.bIsJumpPressed = bWantsToJump;
	
	bJustWantsToJump = false;
	
	FNPMoverInputCmd& NPInputs = Cmd->InputCollection.FindOrAddMutableDataByType<FNPMoverInputCmd>();
	NPInputs.bWantsToSprint = bWantsToSprint;
	NPInputs.bWantsToCrouch = bWantsToCrouch;
	NPInputs.WantsToDodgeDirection = WantsToDodgeDirection;
	
	if (!Mover)
		return;
	
	Mover->ProduceInput(DeltaTimeMS, Cmd);
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::RestoreFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState)
{
	if (!Mover)
		return;

	int32 NewBaseSimTimeMs = 0;
	int32 NextFrameNum = 0;

	switch (UNetworkPredictionWorldManager::ActiveInstance->PreferredDefaultTickingPolicy())
	{
	default:	// fall through
	case ENetworkPredictionTickingPolicy::Fixed:
		{
			const FFixedTickState& FixedTickState = UNetworkPredictionWorldManager::ActiveInstance->GetFixedTickState();
			FNetSimTimeStep TimeStep = FixedTickState.GetNextTimeStep();
			NewBaseSimTimeMs = TimeStep.TotalSimulationTime;
			NextFrameNum = TimeStep.Frame;
		}
		break; 

	case ENetworkPredictionTickingPolicy::Independent:
		{
			const FVariableTickState& VariableTickState = UNetworkPredictionWorldManager::ActiveInstance->GetVariableTickState();
			const FNetSimTimeStep NextVariableTimeStep = VariableTickState.GetNextTimeStep(VariableTickState.Frames[VariableTickState.ConfirmedFrame]);
			NewBaseSimTimeMs = NextVariableTimeStep.TotalSimulationTime;
			NextFrameNum = NextVariableTimeStep.Frame;

		}
		break;
	}

	FMoverTimeStep MoverTimeStep;

	MoverTimeStep.ServerFrame = NextFrameNum;
	MoverTimeStep.BaseSimTimeMs = NewBaseSimTimeMs;
	MoverTimeStep.StepMs = 0;

	Mover->RestoreFrame(SyncState, AuxState, MoverTimeStep);
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::FinalizeFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState)
{
	if (!Mover)
		return;

	const FNetworkPredictionSettings NetworkPredictionSettings = UNetworkPredictionWorldManager::ActiveInstance->GetSettings();
	if (Mover->GetOwnerRole() == ROLE_SimulatedProxy && NetworkPredictionSettings.SimulatedProxyNetworkLOD == ENetworkLOD::Interpolated)
	{
		FMoverInputCmdContext InputCmd;
		Mover->TickInterpolatedSimProxy(Mover->GetLastTimeStep(), InputCmd, Mover, Mover->GetSyncState(), *SyncState, *AuxState);
	}
	
	Mover->FinalizeFrame(SyncState, AuxState);
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::FinalizeSmoothingFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState)
{
	if (!Mover)
		return;

	Mover->FinalizeSmoothingFrame(SyncState, AuxState);
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::InitializeSimulationState(FMoverSyncState* OutSync, FMoverAuxStateContext* OutAux)
{
	if (!Mover)
		return;

	StartingOutSync = OutSync;
	StartingOutAux = OutAux;
	Mover->InitializeSimulationState(StartingOutSync, StartingOutAux);
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SimulationTick(const FNetSimTimeStep& TimeStep, const FMoverInputCmdContext& Input, const FMoverSyncState& InSyncState, const FMoverAuxStateContext& InAuxState, FMoverSyncState& OutSyncState, FMoverAuxStateContext& OutAuxState)
{
	if (!Mover)
		return;

	FMoverTickStartData StartData;
	FMoverTickEndData EndData;

	StartData.InputCmd  = Input;
	StartData.SyncState = InSyncState;
	StartData.AuxState  = InAuxState;

	// Ensure persistent SyncStates are present in the start state for a SimTick.
	for (const FMoverDataPersistence& PersistentSyncEntry : Mover->PersistentSyncStateDataTypes)
	{
		StartData.SyncState.SyncStateCollection.FindOrAddDataByType(PersistentSyncEntry.RequiredType);
	}
	
	FMoverTimeStep MoverTimeStep;

	MoverTimeStep.ServerFrame	= TimeStep.Frame;
	MoverTimeStep.BaseSimTimeMs = TimeStep.TotalSimulationTime;
	MoverTimeStep.StepMs		= TimeStep.StepMS;

	Mover->SimulationTick(MoverTimeStep, StartData, OUT EndData);

	OutSyncState = EndData.SyncState;
	OutAuxState = EndData.AuxState;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::InitializeComponent()
{
	Mover = GetOwner()->FindComponentByClass<UMoverComponent>();
	Super::InitializeComponent();
}

double UNPCharacterMoverNetworkPredictionLiaisonComponent::GetCurrentSimTimeMs()
{
	return NetworkPrediction->GetCurrentSimTimeMs();
}

int32 UNPCharacterMoverNetworkPredictionLiaisonComponent::GetCurrentSimFrame()
{
	return NetworkPrediction->GetCurrentSimFrame();
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::ReadPendingSyncState(FMoverSyncState& OutSyncState)
{
	if (const FNPCharacterNetworkPredictionSyncState* SyncState = NetworkPrediction->GetPendingSyncState())
	{
		OutSyncState = SyncState->Mover;
		return true;
	}

	return false;
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::WritePendingSyncState(const FMoverSyncState& SyncStateToWrite)
{
	return NetworkPrediction->WritePendingSyncState([&SyncStateToWrite](FNPCharacterNetworkPredictionSyncState& SyncStateRef)
	{
		SyncStateRef.Mover = SyncStateToWrite;
	});
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::ReadPresentationSyncState(FMoverSyncState& OutSyncState)
{
	if (const FNPCharacterNetworkPredictionSyncState* SyncState = NetworkPrediction->GetPresentationSyncState())
	{
		OutSyncState = SyncState->Mover;
		return true;
	}

	return false;
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::WritePresentationSyncState(const FMoverSyncState& SyncStateToWrite)
{
	return NetworkPrediction->WritePresentationSyncState([&SyncStateToWrite](FNPCharacterNetworkPredictionSyncState& SyncStateRef)
	{
		SyncStateRef.Mover = SyncStateToWrite;
	});
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::ReadPrevPresentationSyncState(FMoverSyncState& OutSyncState)
{
	if (const FNPCharacterNetworkPredictionSyncState* SyncState = NetworkPrediction->GetPrevPresentationSyncState())
	{
		OutSyncState = SyncState->Mover;
		return true;
	}

	return false;
}

bool UNPCharacterMoverNetworkPredictionLiaisonComponent::WritePrevPresentationSyncState(const FMoverSyncState& SyncStateToWrite)
{
	return NetworkPrediction->WritePrevPresentationSyncState([&SyncStateToWrite](FNPCharacterNetworkPredictionSyncState& SyncStateRef)
	{
		SyncStateRef.Mover = SyncStateToWrite;
	});
}

#if WITH_EDITOR
EDataValidationResult UNPCharacterMoverNetworkPredictionLiaisonComponent::ValidateData(FDataValidationContext& Context, const UMoverComponent& ValidationMoverComp) const
{
	if (const AActor* OwnerActor = ValidationMoverComp.GetOwner())
	{
		if (OwnerActor->IsReplicatingMovement())
		{
			Context.AddError(
				FText::Format(
					FText::FromString("The owning actor ({0}) has the ReplicateMovement property enabled. This will conflict with Network Prediction and cause poor quality movement. Please disable it."),
					FText::FromString(GetNameSafe(OwnerActor))
				)
			);
			return EDataValidationResult::Invalid;
		}
	}

	return EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetMoveDirection(const FVector2D& Direction)
{
	PendingMoveDirection = Direction;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetLookDirection(const FVector2D& Direction)
{
	PendingLookDirection = Direction;
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn))
	{
		OwnerPawn->AddControllerYawInput(Direction.X);
		OwnerPawn->AddControllerPitchInput(Direction.Y);
	}
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetWantsToJump(bool bPressed)
{
	bWantsToJump = bPressed;
	bJustWantsToJump = bPressed;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetWantsToSprint(bool bPressed)
{
	bWantsToSprint = bPressed;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetWantsToCrouch(bool bPressed)
{
	bWantsToCrouch = bPressed;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::SetWantsToDodge(ENPDodgeDirection Direction)
{
	WantsToDodgeDirection = Direction;
}

void UNPCharacterMoverNetworkPredictionLiaisonComponent::OnUnregister()
{
	Mover = nullptr;
	Super::OnUnregister();
}


// Called when the game starts
void UNPCharacterMoverNetworkPredictionLiaisonComponent::BeginPlay()
{
	Super::BeginPlay();

	if (StartingOutSync && StartingOutAux && Mover)
	{
		if (FMoverDefaultSyncState* StartingSyncState = StartingOutSync->SyncStateCollection.FindMutableDataByType<FMoverDefaultSyncState>())
		{
			const FTransform UpdatedComponentTransform = Mover->GetUpdatedComponentTransform();
			// if our location has changed between initialization and begin play (ex: Actors sharing an exact start location and one gets "pushed" to make them fit) lets write the new location to avoid any disagreements
			if (!UpdatedComponentTransform.GetLocation().Equals(StartingSyncState->GetLocation_WorldSpace()))
			{
				StartingSyncState->SetTransforms_WorldSpace(UpdatedComponentTransform.GetLocation(),
													 UpdatedComponentTransform.GetRotation().Rotator(),
													 FVector::ZeroVector,
													 FVector::ZeroVector);	// no initial velocity
			}
		}
	}
}
