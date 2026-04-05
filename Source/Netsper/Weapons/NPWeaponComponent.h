#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPWeaponComponent.generated.h"

class UNPWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSwitched, int32, NewIndex, UNPWeaponBase*, NewWeapon);

/**
 * UNPWeaponComponent — Manages up to 3 weapon slots.
 * Handles equipping, switching, and delegating fire/reload to the active weapon.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPWeaponComponent();

	/** Get the currently active weapon (may return nullptr) */
	UFUNCTION(BlueprintCallable, Category = "NP|Weapon")
	UNPWeaponBase* GetActiveWeapon() const;

	/** Begin primary fire on the active weapon */
	void StartFire();

	/** End primary fire on the active weapon */
	void StopFire();

	/** Begin alt fire on the active weapon */
	void StartAltFire();

	/** Reload the active weapon */
	void Reload();

	/** Switch weapon by direction (+1 next, -1 prev) */
	void SwitchWeapon(int32 Direction);

	UPROPERTY(BlueprintAssignable, Category = "NP|Weapon")
	FOnWeaponSwitched OnWeaponSwitched;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Weapon slots (max 3 per GDD) */
	UPROPERTY(ReplicatedUsing = OnRep_Weapons, BlueprintReadOnly, Category = "NP|Weapon")
	TArray<TObjectPtr<UNPWeaponBase>> WeaponSlots;

	/** Index of the active weapon in WeaponSlots */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveWeaponIndex, BlueprintReadOnly, Category = "NP|Weapon")
	int32 ActiveWeaponIndex = 0;

	UFUNCTION()
	void OnRep_Weapons();

	UFUNCTION()
	void OnRep_ActiveWeaponIndex();

	/** Server RPC to validate weapon switch */
	UFUNCTION(Server, Reliable)
	void Server_SwitchWeapon(int32 NewIndex);

	/** Server RPC for fire validation */
	UFUNCTION(Server, Reliable)
	void Server_Fire(FVector Origin, FRotator Direction, float Timestamp);
};
