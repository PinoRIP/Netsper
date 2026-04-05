#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPMantleMode.generated.h"

/**
 * UNPMantleMode — Ledge grab and pull-up with normal and SP-boosted variants.
 */
UCLASS()
class NETSPER_API UNPMantleMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;
	virtual void Activate() override;
	virtual void Deactivate() override;

protected:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float MantleVerticalSpeed = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float MantleHorizontalSpeed = 260.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float MantleMaxDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float LedgeDetectForwardDistance = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float LedgeDetectUpDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|Tuning")
	float CrouchHalfHeight = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|SP")
	float SPBoostMantleCost = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mantle|SP")
	float SPBoostUpwardImpulse = 800.f;

private:
	FVector LedgeTopPosition = FVector::ZeroVector;
	bool bIsPullingUp = false;
	bool bIsSPBoosted = false;
};
