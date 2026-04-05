#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Stamina/NPStaminaInterface.h"
#include "Stamina/NPStaminaTypes.h"
#include "NPStaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

/**
 * UNPStaminaComponent — Manages the SP (Stamina Points) resource.
 *
 * Canonical SP value lives here. Movement modes read a mirrored copy
 * via FNPMoverState for prediction. Implements INPStaminaProvider.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPStaminaComponent : public UActorComponent, public INPStaminaProvider
{
	GENERATED_BODY()

public:
	UNPStaminaComponent();

	// INPStaminaProvider interface
	virtual float GetCurrentSP() const override;
	virtual float GetMaxSP() const override;
	virtual bool TryConsumeSP(float Amount) override;
	virtual void ConsumeSP(float Amount) override;
	virtual void RestoreSP(float Amount) override;

	/** Broadcast when SP changes */
	UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
	FOnStaminaChanged OnStaminaChanged;

	/** Get current SP fraction (0-1) */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float GetStaminaPercent() const;

	/** Notify that SP was consumed (resets regen delay) */
	void NotifyConsumption();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float MaxSP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float RegenRate = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float RegenDelay = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float SprintRegenPenalty = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Config")
	float CombatRegenPenalty = 0.3f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSP)
	float CurrentSP;

	float RegenDelayRemaining = 0.f;

	UFUNCTION()
	void OnRep_CurrentSP();
};
