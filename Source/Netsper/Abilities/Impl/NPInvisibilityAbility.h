#pragma once

#include "CoreMinimal.h"
#include "Abilities/NPAbilityBase.h"
#include "NPInvisibilityAbility.generated.h"

/**
 * UNPInvisibilityAbility — Temporary stealth.
 *
 * Hides the player mesh, reduces audio, applies shimmer overlay.
 * Breaks on: fire weapon, take damage, or duration expiration.
 */
UCLASS()
class NETSPER_API UNPInvisibilityAbility : public UNPAbilityBase
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetAbilityTag() const override;
	virtual float GetSPCost() const override { return 0.f; }
	virtual int32 GetCooldownMs() const override { return 10000; }
	virtual int32 GetDurationMs() const override { return 5000; }

	virtual void OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
	                         FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
	                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) override;
};
