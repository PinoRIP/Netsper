#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NPAbilityBase.generated.h"

struct FNPAbilityInputCmd;
struct FNPAbilitySyncState;
struct FNPAbilityAuxState;
class UMoverComponent;

/**
 * UNPAbilityBase — Abstract base for all ability implementations.
 *
 * Abilities are UObject subobjects owned by UNPAbilityComponent.
 * They define intrinsic properties (cost, cooldown, duration) and
 * simulation callbacks invoked during the ability component's tick.
 */
UCLASS(Abstract, EditInlineNew)
class NETSPER_API UNPAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	/** Gameplay tag identifying this ability */
	virtual FGameplayTag GetAbilityTag() const PURE_VIRTUAL(UNPAbilityBase::GetAbilityTag, return FGameplayTag(););

	/** SP cost to activate (0 = free, >0 = continuous or one-time) */
	virtual float GetSPCost() const { return 0.f; }

	/** Cooldown in milliseconds */
	virtual int32 GetCooldownMs() const PURE_VIRTUAL(UNPAbilityBase::GetCooldownMs, return 0;);

	/** Duration in milliseconds (0 = instant, >0 = timed) */
	virtual int32 GetDurationMs() const { return 0; }

	// --- Simulation callbacks (called from ability component tick) ---

	/** Called when the ability is activated */
	virtual void OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
	                         FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) {}

	/** Called every tick while active */
	virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
	                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp) {}

	/** Called when the ability is deactivated */
	virtual void OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState) {}

	// --- Server authority callbacks ---

	/** Server-only: spawn world actors on activation (shields, walls, ropes) */
	virtual void OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd& InputCmd) {}

	/** Server-only: clean up world actors on deactivation */
	virtual void OnDeactivatedAuthority(AActor* Owner) {}
};
