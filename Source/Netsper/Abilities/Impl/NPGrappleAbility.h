#pragma once

#include "CoreMinimal.h"
#include "Abilities/NPAbilityBase.h"
#include "NPGrappleAbility.generated.h"

/**
 * UNPGrappleAbility — Line trace + pull toward hook point.
 *
 * On activation, traces forward to find a grapple surface.
 * If hit: attaches hook and pulls the player via a layered move.
 * Adds to momentum — does not halt horizontal velocity.
 * (Apex Legends style: shorter range, repositioning tool.)
 */
UCLASS()
class NETSPER_API UNPGrappleAbility : public UNPAbilityBase
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetAbilityTag() const override;
	virtual float GetSPCost() const override { return 30.f; }
	virtual int32 GetCooldownMs() const override { return 8000; }
	virtual int32 GetDurationMs() const override { return 0; } // Until detach or cancel

	virtual void OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
	                         FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
	                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) override;

	/** Max grapple range (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Grapple")
	float MaxRange = 2000.f;

	/** Distance at which to detach the hook (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Grapple")
	float DetachDistance = 150.f;

	/** Short penalty cooldown on miss (ms) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Grapple")
	int32 MissPenaltyCooldownMs = 1000;
};
