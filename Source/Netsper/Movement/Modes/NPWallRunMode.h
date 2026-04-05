#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPWallRunMode.generated.h"

/**
 * UNPWallRunMode — Side-wall traversal with natural gravity arc.
 */
UCLASS()
class NETSPER_API UNPWallRunMode : public UBaseMovementMode
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Tuning")
	float WallRunMaxDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Tuning")
	float WallRunMinContinueSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Tuning")
	float WallRunGravityDrift = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Tuning")
	float WallTraceDistance = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Jump")
	float WallJumpUpVelocity = 550.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Jump")
	float WallJumpAwayVelocity = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallRun|Tuning")
	float CameraTiltAngle = 12.f;

private:
	FVector CachedWallNormal = FVector::ZeroVector;
	bool bIsLeftWall = false;
};
