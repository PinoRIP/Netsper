#include "Abilities/Impl/NPInvisibilityAbility.h"
#include "Abilities/NPAbilityTypes.h"
#include "MoverComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Netsper.h"

FGameplayTag UNPInvisibilityAbility::GetAbilityTag() const
{
	return FGameplayTag::RequestGameplayTag(FName("Ability.Invisibility"));
}

void UNPInvisibilityAbility::OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
                                         FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	AuxState.bIsInvisible = true;

	if (!IsValid(MoverComp))
	{
		return;
	}

	AActor* Owner = MoverComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// Hide the body mesh (server propagates via replication)
	TArray<USkeletalMeshComponent*> MeshComps;
	Owner->GetComponents<USkeletalMeshComponent>(MeshComps);
	for (USkeletalMeshComponent* MeshComp : MeshComps)
	{
		if (IsValid(MeshComp) && MeshComp->GetFName() == FName("BodyMesh"))
		{
			MeshComp->SetVisibility(false, true);
		}
	}

	// Audio reduction is handled via BlueprintImplementableEvent on the pawn
	UE_LOG(LogNPAbility, Log, TEXT("Invisibility activated for %s"), *Owner->GetName());
}

void UNPInvisibilityAbility::OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
                                    FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	// Break conditions (fire weapon, take damage) are handled externally
	// by the ability component or health component calling RequestCancel.
	// Duration expiration is handled by the ability component's simulation tick.
}

void UNPInvisibilityAbility::OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState)
{
	AuxState.bIsInvisible = false;

	// Note: Re-showing the mesh is done in the ability component's deactivation
	// flow or via OnRep. We set the state here so replication propagates.
	UE_LOG(LogNPAbility, Log, TEXT("Invisibility deactivated"));
}
