// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MoverSimulationTypes.h"
#include "NetworkPredictionComponent.h"
#include "NetworkPredictionSimulation.h"
#include "NetworkPredictionTickState.h"
#include "NPCharacterNetworkPredictionTypes.h"
#include "Components/ActorComponent.h"
#include "NPCharacterNetworkPredictionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NETSPER_API UNPCharacterNetworkPredictionComponent : public UNetworkPredictionComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNPCharacterNetworkPredictionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// UNetworkPredictionComponent interface
	virtual void InitializeNetworkPredictionProxy() override;
	// End UNetworkPredictionComponent interface
	
	virtual void OnUnregister() override;
	virtual void UninitializeComponent() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Begin NP Driver interface
	// Get latest local input prior to simulation step. Called by Network Prediction system on owner's instance (autonomous or authority).
	void ProduceInput(const int32 DeltaTimeMS, FNPCharacterNetworkPredictionInput* Input);

	// Restore a previous frame prior to resimulating. Called by Network Prediction system.
	void RestoreFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState);

	// Take output for simulation. Called by Network Prediction system.
	void FinalizeFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState);

	// Take output for smoothing. Called by Network Prediction system.
	void FinalizeSmoothingFrame(const FNPCharacterNetworkPredictionSyncState* SyncState, const FNPCharacterNetworkPredictionAuxState* AuxState);

	// Seed initial values based on component's state. Called by Network Prediction system.
	void InitializeSimulationState(FNPCharacterNetworkPredictionSyncState* OutSync, FNPCharacterNetworkPredictionAuxState* OutAux);

	// Primary movement simulation update. Given an starting state and timestep, produce a new state. Called by Network Prediction system.
	void SimulationTick(const FNetSimTimeStep& TimeStep, const TNetSimInput<NPCharacterNetworkPredictionTypes>& SimInput, const TNetSimOutput<NPCharacterNetworkPredictionTypes>& SimOutput);

	// End NP Driver interface
	
	
	virtual double GetCurrentSimTimeMs();
	virtual int32 GetCurrentSimFrame();
	
	
	virtual const FNPCharacterNetworkPredictionSyncState* GetPendingSyncState();
	virtual bool WritePendingSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc);
	virtual const FNPCharacterNetworkPredictionSyncState* GetPresentationSyncState();
	virtual bool WritePresentationSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc);
	virtual const FNPCharacterNetworkPredictionSyncState* GetPrevPresentationSyncState();
	virtual bool WritePrevPresentationSyncState(TFunctionRef<void(FNPCharacterNetworkPredictionSyncState&)> WriteFunc);

private:
	UPROPERTY()
	class UNPCharacterMoverNetworkPredictionLiaisonComponent* MoverLiaison = nullptr;
	
	class UNPCharacterMoverNetworkPredictionLiaisonComponent* GetMoverLiaison();
	
};
