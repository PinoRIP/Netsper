// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkPredictionTickState.h"
#include "Backends/MoverBackendLiaison.h"
#include "Components/ActorComponent.h"
#include "Movement/NPMoverTypes.h"
#include "Movement/Input/NPMovementController.h"
#include "NPCharacterMoverNetworkPredictionLiaisonComponent.generated.h"


class UNPCharacterNetworkPredictionComponent;

UCLASS(ClassGroup=(Custom))
class NETSPER_API UNPCharacterMoverNetworkPredictionLiaisonComponent : public UActorComponent, public IMoverBackendLiaisonInterface, public INPMovementController
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNPCharacterMoverNetworkPredictionLiaisonComponent();
	
	// Get latest local input prior to simulation step. Called by UNPCharacterNetworkPredictionComponent
	void ProduceInput(const int32 DeltaTimeMS, FMoverInputCmdContext* Cmd);

	// Restore a previous frame prior to resimulating. Called by UNPCharacterNetworkPredictionComponent
	void RestoreFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState);

	// Take output for simulation. Called by UNPCharacterNetworkPredictionComponent
	void FinalizeFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState);

	// Take output for smoothing. Called by UNPCharacterNetworkPredictionComponent
	void FinalizeSmoothingFrame(const FMoverSyncState* SyncState, const FMoverAuxStateContext* AuxState);

	// Seed initial values based on component's state. Called by UNPCharacterNetworkPredictionComponent
	void InitializeSimulationState(FMoverSyncState* OutSync, FMoverAuxStateContext* OutAux);

	// Primary movement simulation update. Given an starting state and timestep, produce a new state. Called byUNPCharacterNetworkPredictionComponent
	void SimulationTick(const FNetSimTimeStep& TimeStep, const FMoverInputCmdContext& Input, const FMoverSyncState& InSyncState, const FMoverAuxStateContext& InAuxState, FMoverSyncState& OutSyncState, FMoverAuxStateContext& OutAuxState);
	// End NP Driver interface

	virtual void InitializeComponent() override;
	
	// IMoverBackendLiaisonInterface
	virtual double GetCurrentSimTimeMs() override;
	virtual int32 GetCurrentSimFrame() override;
	virtual bool ReadPendingSyncState(OUT FMoverSyncState& OutSyncState) override;
	virtual bool WritePendingSyncState(const FMoverSyncState& SyncStateToWrite) override;
	virtual bool ReadPresentationSyncState(OUT FMoverSyncState& OutSyncState) override;
	virtual bool WritePresentationSyncState(const FMoverSyncState& SyncStateToWrite) override;
	virtual bool ReadPrevPresentationSyncState(FMoverSyncState& OutSyncState) override;
	virtual bool WritePrevPresentationSyncState(const FMoverSyncState& SyncStateToWrite) override;
#if WITH_EDITOR
	virtual EDataValidationResult ValidateData(FDataValidationContext& Context, const UMoverComponent& ValidationMoverComp) const override;
#endif
	// End IMoverBackendLiaisonInterface
	
	
	// INPMovementController
	virtual void SetMoveDirection(const FVector2D& Direction) override;
	virtual void SetLookDirection(const FVector2D& Direction) override;
	virtual void SetWantsToJump(bool bPressed) override;
	virtual void SetWantsToSprint(bool bPressed) override;
	virtual void SetWantsToCrouch(bool bPressed) override;
	virtual void SetWantsToDodge(ENPDodgeDirection Direction) override;
	// End INPMovementController
	
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void OnUnregister() override;
	
private:
	UPROPERTY()
	TObjectPtr<UMoverComponent> Mover;
	
	UPROPERTY()
	TObjectPtr<UNPCharacterNetworkPredictionComponent> NetworkPrediction;
	
	
	FVector2D PendingMoveDirection;
	FVector2D PendingLookDirection;
	
	bool bJustWantsToJump;
	bool bWantsToJump;
	bool bWantsToSprint;
	bool bWantsToCrouch;
	ENPDodgeDirection WantsToDodgeDirection;
	
	
	
	FMoverSyncState* StartingOutSync;
	FMoverAuxStateContext* StartingOutAux;
};
