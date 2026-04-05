#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Abilities/NPAbilityTypes.h"
#include "NPAbilityComponent.generated.h"

class UNPAbilityBase;
class UMoverComponent;
class UNPStaminaComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityActivated, FGameplayTag, AbilityTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityDeactivated, FGameplayTag, AbilityTag);

/**
 * UNPAbilityComponent — Manages equipped ability, runs NPP-style simulation tick.
 *
 * Owns the active UNPAbilityBase instance. Ticks before MoverComponent so that
 * ability-injected layered moves are consumed in the same frame.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPAbilityComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Set the ability class to equip. Creates the ability instance. */
	UFUNCTION(BlueprintCallable, Category = "NP|Ability")
	void EquipAbility(TSubclassOf<UNPAbilityBase> AbilityClass);

	/** Request ability activation (called from input) */
	void RequestActivation(uint8 Slot = 1);

	/** Request cancellation of active ability */
	void RequestCancel();

	/** Get the current ability state */
	UFUNCTION(BlueprintCallable, Category = "NP|Ability")
	const FNPAbilitySyncState& GetAbilityState() const { return AbilityState; }

	/** Get the equipped ability */
	UFUNCTION(BlueprintCallable, Category = "NP|Ability")
	UNPAbilityBase* GetEquippedAbility() const { return EquippedAbility; }

	/** Is any ability currently active? */
	UFUNCTION(BlueprintCallable, Category = "NP|Ability")
	bool IsAbilityActive() const { return AbilityState.bAbilityActive; }

	UPROPERTY(BlueprintAssignable, Category = "NP|Ability")
	FOnAbilityActivated OnAbilityActivated;

	UPROPERTY(BlueprintAssignable, Category = "NP|Ability")
	FOnAbilityDeactivated OnAbilityDeactivated;

protected:
	/** Run the ability simulation for one tick */
	void SimulationTick(int32 DeltaMs);

	/** Activate the equipped ability */
	void ActivateAbility(const FNPAbilityInputCmd& InputCmd);

	/** Deactivate the currently active ability */
	void DeactivateAbility();

	UPROPERTY()
	TObjectPtr<UNPAbilityBase> EquippedAbility;

	UPROPERTY(ReplicatedUsing = OnRep_AbilityState, BlueprintReadOnly, Category = "NP|Ability")
	FNPAbilitySyncState AbilityState;

	UPROPERTY()
	FNPAbilityAuxState AuxState;

	/** Pending input for the current frame */
	FNPAbilityInputCmd PendingInput;

	UFUNCTION()
	void OnRep_AbilityState();

	/** Cached references */
	UPROPERTY()
	TObjectPtr<UMoverComponent> CachedMoverComponent;

	UPROPERTY()
	TObjectPtr<UNPStaminaComponent> CachedStaminaComponent;
};
