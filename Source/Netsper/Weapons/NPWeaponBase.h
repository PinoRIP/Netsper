#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NPWeaponBase.generated.h"

/**
 * UNPWeaponBase — Abstract base for all weapons (ranged and melee).
 */
UCLASS(Abstract)
class NETSPER_API UNPWeaponBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void StartFire(const FVector& Origin, const FRotator& Direction) {}
	virtual void StopFire() {}
	virtual void StartAltFire() {}
	virtual void Reload() {}
	virtual bool CanFire() const { return CurrentAmmo > 0; }
	virtual FGameplayTag GetWeaponTag() const { return FGameplayTag(); }

	virtual void OnEquipped() {}
	virtual void OnUnequipped() {}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float BaseDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float FireRate = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 AmmoPerClip = 30;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float ReloadTime = 1.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsReloading = false;
};
