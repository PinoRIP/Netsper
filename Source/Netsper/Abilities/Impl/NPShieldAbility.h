#pragma once

#include "CoreMinimal.h"
#include "Abilities/NPAbilityBase.h"
#include "NPShieldAbility.generated.h"

class AActor;

/**
 * UNPShieldAbility — Front-facing deployable shield.
 *
 * Spawns a shield actor attached to the player that blocks projectiles.
 * Has its own HP pool separate from player HP.
 */
UCLASS()
class NETSPER_API UNPShieldAbility : public UNPAbilityBase
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetAbilityTag() const override;
	virtual float GetSPCost() const override { return 0.f; }
	virtual int32 GetCooldownMs() const override { return 12000; }
	virtual int32 GetDurationMs() const override { return 5000; }

	virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
	                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) override;
	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) override;

	virtual void OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd& InputCmd) override;
	virtual void OnDeactivatedAuthority(AActor* Owner) override;

	/** Shield HP pool */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Shield")
	float ShieldMaxHP = 200.f;

	/** Blueprint class for the shield actor (assigned in editor) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Shield")
	TSubclassOf<AActor> ShieldActorClass;

private:
	/** Cached reference to spawned shield actor (server only) */
	TWeakObjectPtr<AActor> SpawnedShieldActor;
};
