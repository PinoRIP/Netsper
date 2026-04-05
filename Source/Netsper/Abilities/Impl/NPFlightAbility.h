#pragma once

#include "CoreMinimal.h"
#include "Abilities/NPAbilityBase.h"
#include "NPFlightAbility.generated.h"

/**
 * UNPFlightAbility — Continuous SP-drain flight.
 *
 * While active, overrides gravity and allows 6DoF flight.
 * Deactivates when SP runs out.
 * Cooldown only triggers when fuel (SP) is depleted, not on manual cancel.
 */
UCLASS()
class NETSPER_API UNPFlightAbility : public UNPAbilityBase
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetAbilityTag() const override;
	virtual float GetSPCost() const override { return 0.f; } // Continuous drain, not upfront
	virtual int32 GetCooldownMs() const override { return 6000; }
	virtual int32 GetDurationMs() const override { return 0; } // Unlimited, limited by SP

	virtual void OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
	                         FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
	                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) override;

	/** SP drain per second while flying */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Flight")
	float FlightDrainRate = 25.f;

private:
	/** Whether deactivation is caused by fuel running out (triggers cooldown) */
	bool bFuelDepleted = false;
};
