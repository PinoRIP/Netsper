#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Health/NPDamageInterface.h"
#include "NPHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDied, AActor*, Instigator);

/**
 * UNPHealthComponent — Standard replicated health. Not NPP-predicted.
 * Implements INPDamageable for damage/heal interface.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPHealthComponent : public UActorComponent, public INPDamageable
{
	GENERATED_BODY()

public:
	UNPHealthComponent();

	// INPDamageable interface
	virtual void ApplyDamage(float Amount, AActor* Instigator, FGameplayTag DamageType) override;
	virtual void ApplyHeal(float Amount) override;
	virtual bool IsAlive() const override;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FOnPlayerDied OnPlayerDied;

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthPercent() const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health|Config")
	float MaxHealth = 100.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	UFUNCTION()
	void OnRep_CurrentHealth();
};
