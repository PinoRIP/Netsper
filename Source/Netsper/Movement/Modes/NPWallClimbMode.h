#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "NPWallClimbMode.generated.h"

/**
 * UNPWallClimbMode — Short vertical climb on a wall, facing it.
 */
UCLASS()
class NETSPER_API UNPWallClimbMode : public UBaseMovementMode
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Tuning")
	float ClimbSpeed = 480.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Tuning")
	float WallClimbMaxDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Tuning")
	float WallTraceDistance = 55.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Tuning")
	float TopClearanceBoost = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Jump")
	float MiniJumpUpVelocity = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WallClimb|Jump")
	float MiniJumpForwardVelocity = 300.f;

private:
	FVector CachedWallNormal = FVector::ZeroVector;
};
