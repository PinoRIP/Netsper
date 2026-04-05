#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPSlideMode.generated.h"

/**
 * UNPSlideMode — Momentum-preserving slide entered from ground sprint + crouch.
 */
UCLASS()
class NETSPER_API UNPSlideMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

protected:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float SlideFriction = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float SlideExitSpeed = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float DownhillBoostScale = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float UphillFrictionScale = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float CrouchHalfHeight = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float MaxWalkSlopeCosine = 0.71f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float FloorSweepDistance = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slide|Tuning")
	float MaxWalkSpeed = 600.f;
};
