#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPAirMovementMode.generated.h"

struct FCharacterDefaultInputs;
struct FNPMoverInputCmd;
struct FNPMoverState;

/**
 * UNPAirMovementMode — Handles falling, air control, coyote time, and variable-height jump.
 */
UCLASS()
class NETSPER_API UNPAirMovementMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	virtual void OnRegistered(const FName ModeName) override;
	virtual void OnUnregistered() override;

protected:
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;

	// Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float AirControlMultiplier = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float GravityScale = 1.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float TerminalVelocity = -3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float MaxAirSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float AirAcceleration = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Jump")
	float JumpInitialVelocity = 680.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Jump")
	float JumpHoldGravityScale = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Jump")
	float JumpHoldMaxDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float CoyoteTimeDuration = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Landing")
	float StaggerThreshold = 800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Landing")
	float StaggerDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Landing")
	float LandingTraceDistance = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float MaxWalkSlopeCosine = 0.71f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|Tuning")
	float FloorSweepDistance = 40.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|WallRun")
	float WallRunMinEntrySpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Air|WallRun")
	float WallTraceDistance = 55.f;
};
