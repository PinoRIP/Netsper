#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NPAbilityTypes.generated.h"

/**
 * FNPAbilityInputCmd — Ability-specific input sent from client to server per tick.
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPAbilityInputCmd
{
	GENERATED_BODY()

	/** Which ability slot to activate (0 = none) */
	UPROPERTY()
	uint8 ActivationSlot = 0;

	/** Request cancellation of current active ability */
	UPROPERTY()
	bool bCancelActive = false;
};

/**
 * FNPAbilitySyncState — Predicted/authoritative ability state.
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPAbilitySyncState
{
	GENERATED_BODY()

	/** Is an ability currently active? */
	UPROPERTY()
	bool bAbilityActive = false;

	/** Tag of the currently active ability */
	UPROPERTY()
	FGameplayTag ActiveAbilityTag;

	/** Cooldown remaining in milliseconds */
	UPROPERTY()
	int32 AbilityCooldownRemainingMs = 0;

	/** Duration remaining in milliseconds (0 = instant or expired) */
	UPROPERTY()
	int32 AbilityDurationRemainingMs = 0;
};

/**
 * FNPAbilityAuxState — Non-predicted ability state (server-only cosmetic/auth data).
 */
USTRUCT(BlueprintType)
struct NETSPER_API FNPAbilityAuxState
{
	GENERATED_BODY()

	/** Grapple hook attachment point (world space) */
	UPROPERTY()
	FVector GrappleHookPoint = FVector::ZeroVector;

	/** Whether the grapple hook is currently attached */
	UPROPERTY()
	bool bGrappleHookAttached = false;

	/** Shield HP remaining */
	UPROPERTY()
	float ShieldHP = 0.f;

	/** Wall HP remaining */
	UPROPERTY()
	float WallHP = 0.f;

	/** Invisibility active flag */
	UPROPERTY()
	bool bIsInvisible = false;
};
