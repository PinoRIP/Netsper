#pragma once

#include "CoreMinimal.h"
#include "Abilities/NPAbilityBase.h"
#include "NPWallAbility.generated.h"

/**
 * UNPWallAbility — Deployable cover wall.
 *
 * Spawns a wall actor in front of the player, aligned to the floor.
 * The wall blocks projectiles and movement, has its own HP pool.
 */
UCLASS()
class NETSPER_API UNPWallAbility : public UNPAbilityBase
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetAbilityTag() const override;
	virtual float GetSPCost() const override { return 0.f; }
	virtual int32 GetCooldownMs() const override { return 15000; }
	virtual int32 GetDurationMs() const override { return 8000; }

	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) override;

	virtual void OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd& InputCmd) override;
	virtual void OnDeactivatedAuthority(AActor* Owner) override;

	/** Wall max HP */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Wall")
	float WallMaxHP = 300.f;

	/** How far in front of the player to place the wall (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Wall")
	float PlacementDistance = 200.f;

	/** Blueprint class for the wall actor (assigned in editor) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Wall")
	TSubclassOf<AActor> WallActorClass;

private:
	TWeakObjectPtr<AActor> SpawnedWallActor;
};
