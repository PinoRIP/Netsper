#include "Abilities/Impl/NPGrappleAbility.h"
#include "Abilities/NPAbilityTypes.h"
#include "MoverComponent.h"
#include "Movement/LayeredMoves/NPGrappleLayeredMove.h"
#include "GameFramework/Pawn.h"
#include "Netsper.h"

FGameplayTag UNPGrappleAbility::GetAbilityTag() const
{
	return FGameplayTag::RequestGameplayTag(FName("Ability.Grapple"));
}

void UNPGrappleAbility::OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
                                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	if (!IsValid(MoverComp))
	{
		return;
	}

	AActor* Owner = MoverComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// Get view point for trace
	FVector TraceStart = FVector::ZeroVector;
	FRotator TraceRot = FRotator::ZeroRotator;

	APawn* Pawn = Cast<APawn>(Owner);
	if (IsValid(Pawn))
	{
		TraceStart = Pawn->GetPawnViewLocation();
		TraceRot = Pawn->GetViewRotation();
	}
	else
	{
		TraceStart = Owner->GetActorLocation();
		TraceRot = Owner->GetActorRotation();
	}

	const FVector TraceEnd = TraceStart + TraceRot.Vector() * MaxRange;

	// Line trace to find grapple target
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	const bool bHit = Owner->GetWorld()->LineTraceSingleByChannel(
		HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

	if (bHit)
	{
		// Attach hook
		AuxState.GrappleHookPoint = HitResult.ImpactPoint;
		AuxState.bGrappleHookAttached = true;

		// Queue grapple layered move
		TSharedPtr<FNPGrappleLayeredMove> GrappleMove = MakeShared<FNPGrappleLayeredMove>();
		GrappleMove->HookPoint = HitResult.ImpactPoint;
		GrappleMove->DetachDistance = DetachDistance;
		GrappleMove->DurationMs = 0.f; // Infinite until detach
		MoverComp->QueueLayeredMove(GrappleMove);

		UE_LOG(LogNPAbility, Log, TEXT("Grapple attached at %s"), *HitResult.ImpactPoint.ToString());
	}
	else
	{
		// Miss: fail instantly, apply short penalty cooldown
		AuxState.bGrappleHookAttached = false;
		SyncState.AbilityDurationRemainingMs = 0; // Force deactivation
		SyncState.AbilityCooldownRemainingMs = MissPenaltyCooldownMs;
		SyncState.bAbilityActive = false;

		UE_LOG(LogNPAbility, Log, TEXT("Grapple missed — short penalty cooldown"));
	}
}

void UNPGrappleAbility::OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
                               FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	if (!AuxState.bGrappleHookAttached)
	{
		// Detached — signal deactivation
		SyncState.AbilityDurationRemainingMs = 0;
		return;
	}

	if (!IsValid(MoverComp))
	{
		return;
	}

	// Check distance to hook
	const FVector CurrentLocation = MoverComp->GetUpdatedComponent()->GetComponentLocation();
	const float DistToHook = FVector::Dist(CurrentLocation, AuxState.GrappleHookPoint);

	if (DistToHook < DetachDistance)
	{
		// Arrived — detach
		AuxState.bGrappleHookAttached = false;
		SyncState.AbilityDurationRemainingMs = 0;
		UE_LOG(LogNPAbility, Log, TEXT("Grapple reached hook point — detaching"));
		return;
	}

	// Re-queue grapple move to keep pulling
	TSharedPtr<FNPGrappleLayeredMove> GrappleMove = MakeShared<FNPGrappleLayeredMove>();
	GrappleMove->HookPoint = AuxState.GrappleHookPoint;
	GrappleMove->DetachDistance = DetachDistance;
	GrappleMove->DurationMs = static_cast<float>(DeltaMs);
	MoverComp->QueueLayeredMove(GrappleMove);
}

void UNPGrappleAbility::OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState)
{
	AuxState.bGrappleHookAttached = false;
	AuxState.GrappleHookPoint = FVector::ZeroVector;
}
