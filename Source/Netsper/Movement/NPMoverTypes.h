#pragma once

#include "CoreMinimal.h"
#include "MoverTypes.h"
#include "GameplayTagContainer.h"
#include "NPMoverTypes.generated.h"

/** Movement sub-states for the NP character */
UENUM(BlueprintType)
enum class ENPMovementState : uint8
{
	Ground    UMETA(DisplayName = "Ground"),
	Air       UMETA(DisplayName = "Air"),
	Slide     UMETA(DisplayName = "Slide"),
	WallRun   UMETA(DisplayName = "Wall Run"),
	WallClimb UMETA(DisplayName = "Wall Climb"),
	Mantle    UMETA(DisplayName = "Mantle"),
};

UENUM(BlueprintType)
enum class ENPDodgeDirection : uint8
{
	None  UMETA(DisplayName = "None"),
	Left  UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
};

// -----------------------------------------------
// FNPMoverInputCmd — Custom input data for Mover
// Added alongside FCharacterDefaultInputs in ProduceInput
// -----------------------------------------------

USTRUCT(BlueprintType)
struct NETSPER_API FNPMoverInputCmd : public FMoverDataStructBase
{
	GENERATED_BODY()

	UPROPERTY()
	bool bWantsToSprint = false;

	UPROPERTY()
	bool bWantsToCrouch = false;

	UPROPERTY()
	ENPDodgeDirection WantsToDodgeDirection = ENPDodgeDirection::None;

	// FMoverDataStructBase overrides
	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override {}
};

template<>
struct TStructOpsTypeTraits<FNPMoverInputCmd> : public TStructOpsTypeTraitsBase2<FNPMoverInputCmd>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true,
	};
};

// -----------------------------------------------
// FNPMoverState — Custom sync state for NP movement
// -----------------------------------------------

USTRUCT(BlueprintType)
struct NETSPER_API FNPMoverState : public FMoverDataStructBase
{
	GENERATED_BODY()

	/** Current stamina (mirrored for prediction) */
	UPROPERTY()
	float CurrentSP = 100.f;

	UPROPERTY()
	float MaxSP = 100.f;

	/** Active movement sub-state tag */
	UPROPERTY()
	FGameplayTag MovementSubState;

	/** Time spent in current mode (seconds) */
	UPROPERTY()
	float ModeElapsedTime = 0.f;

	/** Stagger duration remaining (seconds) */
	UPROPERTY()
	float StaggerTimeRemaining = 0.f;

	/** Whether character is currently crouching */
	UPROPERTY()
	bool bIsCrouching = false;

	/** Current capsule half-height */
	UPROPERTY()
	float CapsuleHalfHeight = 48.f;

	/** Air dodge used this airborne phase */
	UPROPERTY()
	bool bAirDodgeUsed = false;

	/** Coyote time remaining (seconds) */
	UPROPERTY()
	float CoyoteTimeRemaining = 0.f;

	/** Jump hold time remaining (seconds) */
	UPROPERTY()
	float JumpHoldTimeRemaining = 0.f;

	/** Whether we are currently holding jump (for variable height) */
	UPROPERTY()
	bool bIsJumping = false;

	/** Time remaining before SP starts regenerating (seconds) */
	UPROPERTY()
	float RegenDelayRemaining = 0.f;

	// FMoverDataStructBase overrides
	virtual FMoverDataStructBase* Clone() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override {}
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthoritativeState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;
};

template<>
struct TStructOpsTypeTraits<FNPMoverState> : public TStructOpsTypeTraitsBase2<FNPMoverState>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true,
	};
};

// -----------------------------------------------
// Movement mode name constants
// -----------------------------------------------

namespace NPMovementModeNames
{
	const FName Ground = TEXT("NPGround");
	const FName Air = TEXT("NPAir");
	const FName Slide = TEXT("NPSlide");
	const FName WallRun = TEXT("NPWallRun");
	const FName WallClimb = TEXT("NPWallClimb");
	const FName Mantle = TEXT("NPMantle");
}

class USceneComponent;

// -----------------------------------------------
// SP prediction utilities — called from movement mode SimulationTick
// -----------------------------------------------

namespace NPStaminaUtils
{
	/** Default regen rate (SP per second) */
	constexpr float DefaultRegenRate = 18.f;

	/** Default delay before regen starts after consumption (seconds) */
	constexpr float DefaultRegenDelay = 1.2f;

	/**
	 * Apply pending ability SP cost and tick SP regen.
	 * @param PendingAbilityCost - one-shot cost from abilities (already flushed from stamina component)
	 */
	FORCEINLINE void TickSP(FNPMoverState& State, float PendingAbilityCost, float DeltaSeconds)
	{
		// 1. Apply pending ability SP cost
		if (PendingAbilityCost > 0.f)
		{
			State.CurrentSP = FMath::Max(0.f, State.CurrentSP - PendingAbilityCost);
			State.RegenDelayRemaining = DefaultRegenDelay;
		}

		// 2. Regen: tick delay, then restore SP
		if (State.RegenDelayRemaining > 0.f)
		{
			State.RegenDelayRemaining = FMath::Max(0.f, State.RegenDelayRemaining - DeltaSeconds);
		}
		else if (State.CurrentSP < State.MaxSP)
		{
			State.CurrentSP = FMath::Min(State.CurrentSP + DefaultRegenRate * DeltaSeconds, State.MaxSP);
		}
	}

	/** Reset regen delay after SP consumption. Call when a mode consumes SP. */
	FORCEINLINE void NotifyConsumption(FNPMoverState& State)
	{
		State.RegenDelayRemaining = DefaultRegenDelay;
	}

	/**
	 * Convenience: flush pending ability cost from UNPStaminaComponent and tick SP.
	 * Resolves the stamina component from UpdatedComponent's owner each call.
	 */
	void TickSPFromComponent(FNPMoverState& State, USceneComponent* UpdatedComponent, float DeltaSeconds);
}
