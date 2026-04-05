#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stamina/NPStaminaInterface.h"
#include "NPStaminaComponent.generated.h"

class UMoverComponent;
struct FMoverTimeStep;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

/**
 * UNPStaminaComponent — Predicted SP bridge over Mover 2.0 sync state.
 *
 * The authoritative predicted SP lives in FNPMoverState::CurrentSP, which is
 * predicted and reconciled through NPP. This component:
 *  - Reads SP from the Mover sync state for external queries (UI, abilities)
 *  - Accumulates ability SP costs into PendingAbilitySPCost, which the input
 *    component pipes into FNPMoverInputCmd for predicted consumption
 *  - Fires OnStaminaChanged for UI binding
 *  - SP regen runs inside each movement mode's SimulationTick via NPStaminaUtils::TickSP
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPStaminaComponent : public UActorComponent, public INPStaminaProvider
{
	GENERATED_BODY()

public:
	UNPStaminaComponent();

	// INPStaminaProvider interface
	virtual float GetCurrentSP() const override;
	virtual float GetMaxSP() const override;
	virtual bool TryConsumeSP(float Amount) override;
	virtual void ConsumeSP(float Amount) override;
	virtual void RestoreSP(float Amount) override;

	/** Broadcast when SP changes */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnStaminaChanged OnStaminaChanged;

	/** Get current SP fraction (0-1) */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float GetStaminaPercent() const;

	/**
	 * Flush accumulated ability SP cost. Called by the input component
	 * during ProduceInput to pipe into FNPMoverInputCmd::AbilitySPCost.
	 * Returns the accumulated cost and resets to 0.
	 */
	float FlushPendingAbilitySPCost();

protected:
	virtual void BeginPlay() override;

	/** Config — kept for reference and potential future use */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float MaxSP = 100.f;

private:
	/** Read current SP from Mover sync state (returns MaxSP if Mover unavailable) */
	float GetSPFromMoverState() const;

	/** Cached reference to MoverComponent */
	UPROPERTY()
	TObjectPtr<UMoverComponent> CachedMoverComponent;

	/** Accumulated SP cost from ability activations/drains, flushed per frame into input cmd */
	float PendingAbilitySPCost = 0.f;

	/** Last known SP value for change detection */
	float LastBroadcastSP = -1.f;

	/** Called after Mover finishes a simulation tick — fires OnStaminaChanged */
	UFUNCTION()
	void OnMoverPostSimulationTick(const FMoverTimeStep& TimeStep);
};
