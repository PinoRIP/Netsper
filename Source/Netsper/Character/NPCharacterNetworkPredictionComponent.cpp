// Fill out your copyright notice in the Description page of Project Settings.


#include "NPCharacterNetworkPredictionComponent.h"
#include "NetworkPredictionModelDefRegistry.h"
#include "NetworkPredictionProxyInit.h"
#include "NetworkPredictionProxyWrite.h"
#include "NPCharacterMoverNetworkPredictionLiaisonComponent.h"
#include "NPCharacterNetworkPredictionTypes.h"


class FNPCharacterNetworkPredictionModelDef : public FNetworkPredictionModelDef
{
public:
	NP_MODEL_BODY();

	using Simulation = UNPCharacterNetworkPredictionComponent;
	using StateTypes = NPCharacterNetworkPredictionTypes;
	using Driver = UNPCharacterNetworkPredictionComponent;

	static const TCHAR* GetName() { return TEXT("MoverActor"); }
	static constexpr int32 GetSortPriority() { return (int32)ENetworkPredictionSortPriority::PreKinematicMovers; }
};

NP_MODEL_REGISTER(FNPCharacterNetworkPredictionModelDef);

// Sets default values for this component's properties
UNPCharacterNetworkPredictionComponent::UNPCharacterNetworkPredictionComponent()
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;

	bWantsInitializeComponent = true;
	bAutoActivate = true;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UNPCharacterNetworkPredictionComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UNPCharacterNetworkPredictionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UNPCharacterNetworkPredictionComponent::InitializeNetworkPredictionProxy()
{
	NetworkPredictionProxy.Init<FNPCharacterNetworkPredictionModelDef>(GetWorld(), GetReplicationProxies(), this, this);
}

void UNPCharacterNetworkPredictionComponent::OnUnregister()
{
	MoverLiaison = nullptr;
	
	Super::OnUnregister();
}

void UNPCharacterNetworkPredictionComponent::UninitializeComponent()
{
	NetworkPredictionProxy.EndPlay();
	
	Super::UninitializeComponent();
}

void UNPCharacterNetworkPredictionComponent::ProduceInput(const int32 DeltaTimeMS, FNPCharacterNetworkPredictionInput* Input)
{
	if (MoverLiaison)
	{
		MoverLiaison->ProduceInput(DeltaTimeMS, &Input->Mover);
	}
}

void UNPCharacterNetworkPredictionComponent::RestoreFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState)
{
	if (MoverLiaison)
	{
		MoverLiaison->RestoreFrame(&SyncState->Mover, &AuxState->Mover);
	}
}

void UNPCharacterNetworkPredictionComponent::FinalizeFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState)
{
	if (MoverLiaison)
	{
		MoverLiaison->FinalizeFrame(&SyncState->Mover, &AuxState->Mover);
	}
}

void UNPCharacterNetworkPredictionComponent::FinalizeSmoothingFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState)
{
	if (MoverLiaison)
	{
		MoverLiaison->FinalizeSmoothingFrame(&SyncState->Mover, &AuxState->Mover);
	}
}

void UNPCharacterNetworkPredictionComponent::InitializeSimulationState(FNPCharacterNetworkPredictionSyncState* OutSync, FNPCharacterNetworkPredictionAuxState* OutAux)
{
	if (MoverLiaison)
	{
		MoverLiaison->InitializeSimulationState(&OutSync->Mover, &OutAux->Mover);
	}
}

void UNPCharacterNetworkPredictionComponent::SimulationTick(const FNetSimTimeStep& TimeStep, const TNetSimInput<NPCharacterNetworkPredictionTypes>& SimInput, const TNetSimOutput<NPCharacterNetworkPredictionTypes>& SimOutput)
{
	if (MoverLiaison)
	{
		MoverLiaison->SimulationTick(TimeStep, SimInput.Cmd->Mover, SimInput.Sync->Mover, SimInput.Aux->Mover, SimOutput.Sync->Mover, SimOutput.Aux.Get()->Mover);
	}
}

double UNPCharacterNetworkPredictionComponent::GetCurrentSimTimeMs()
{
	return NetworkPredictionProxy.GetTotalSimTimeMS();
}

int32 UNPCharacterNetworkPredictionComponent::GetCurrentSimFrame()
{
	return NetworkPredictionProxy.GetPendingFrame();
}

const FNPCharacterNetworkPredictionSyncState* UNPCharacterNetworkPredictionComponent::GetPendingSyncState()
{
	return NetworkPredictionProxy.ReadSyncState<FNPCharacterNetworkPredictionSyncState>(ENetworkPredictionStateRead::Simulation);
}

bool UNPCharacterNetworkPredictionComponent::WritePendingSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc)
{
	return NetworkPredictionProxy.WriteSyncState<FNPCharacterNetworkPredictionSyncState>(WriteFunc) != nullptr;
}

const FNPCharacterNetworkPredictionSyncState* UNPCharacterNetworkPredictionComponent::GetPresentationSyncState()
{
	return NetworkPredictionProxy.ReadSyncState<FNPCharacterNetworkPredictionSyncState>(ENetworkPredictionStateRead::Presentation);
}

bool UNPCharacterNetworkPredictionComponent::WritePresentationSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc)
{
	return NetworkPredictionProxy.WritePresentationSyncState<FNPCharacterNetworkPredictionSyncState>(WriteFunc) != nullptr;
}

const FNPCharacterNetworkPredictionSyncState* UNPCharacterNetworkPredictionComponent::GetPrevPresentationSyncState()
{
	return NetworkPredictionProxy.ReadPrevPresentationSyncState<FNPCharacterNetworkPredictionSyncState>();
}

bool UNPCharacterNetworkPredictionComponent::WritePrevPresentationSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc)
{
	return NetworkPredictionProxy.WritePrevPresentationSyncState<FNPCharacterNetworkPredictionSyncState>(WriteFunc) != nullptr;
}

class UNPCharacterMoverNetworkPredictionLiaisonComponent* UNPCharacterNetworkPredictionComponent::GetMoverLiaison()
{
	if (!MoverLiaison)
	{
		MoverLiaison = GetOwner()->FindComponentByClass<UNPCharacterMoverNetworkPredictionLiaisonComponent>();
	}
	
	return MoverLiaison;
}

