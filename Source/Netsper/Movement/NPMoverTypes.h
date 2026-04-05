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

// -----------------------------------------------
// FNPMoverInputCmd — Custom input data for Mover
// Added alongside FCharacterDefaultInputs in ProduceInput
// -----------------------------------------------

USTRUCT(BlueprintType)
struct NETSPER_API FNPMoverInputCmd : public FMoverDataStructBase
{
	GENERATED_BODY()

	UPROPERTY()
	bool bWantsSprint = false;

	UPROPERTY()
	bool bWantsCrouch = false;

	UPROPERTY()
	bool bWantsDodge = false;

	UPROPERTY()
	bool bWantsMantle = false;

	UPROPERTY()
	bool bWantsAbility = false;

	UPROPERTY()
	bool bCancelAbility = false;

	UPROPERTY()
	uint8 RequestedAbilitySlot = 0;

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
