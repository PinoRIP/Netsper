#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPGroundMovementMode.generated.h"

struct FFloorCheckResult;
struct FCharacterDefaultInputs;
struct FNPMoverInputCmd;
struct FNPMoverState;

/**
 * UNPGroundMovementMode — Handles walking, sprinting, and crouching.
 *
 * Sprint and crouch are sub-states within this mode to avoid unnecessary mode transitions.
 */
UCLASS()
class NETSPER_API UNPGroundMovementMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

protected:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	// Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxWalkSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxSprintSpeed = 950.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxCrouchSpeed = 320.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float WalkAcceleration = 2800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float SprintAcceleration = 3200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float BrakingDeceleration = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float GroundFriction = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxWalkSlopeCosine = 0.71f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float FloorSweepDistance = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxStepHeight = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Stamina")
	float SprintSPCostPerSecond = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float SlideEntryThreshold = 700.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float CrouchHalfHeight = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float StandingHalfHeight = 48.f;

private:
	float GetTargetSpeed(const FNPMoverInputCmd* NPInput, const FNPMoverState* NPState) const;
	float GetAcceleration(const FNPMoverInputCmd* NPInput) const;
};
