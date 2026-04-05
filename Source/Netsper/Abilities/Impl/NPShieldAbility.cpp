#include "Abilities/Impl/NPShieldAbility.h"
#include "Abilities/NPAbilityTypes.h"
#include "MoverComponent.h"
#include "GameFramework/Pawn.h"
#include "Netsper.h"

FGameplayTag UNPShieldAbility::GetAbilityTag() const
{
	return FGameplayTag::RequestGameplayTag(FName("Ability.Shield"));
}

void UNPShieldAbility::OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd& InputCmd)
{
	if (!IsValid(Owner) || !ShieldActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 80.f;
	const FRotator SpawnRotation = Owner->GetActorRotation();

	AActor* Shield = Owner->GetWorld()->SpawnActor<AActor>(ShieldActorClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (IsValid(Shield))
	{
		Shield->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
		SpawnedShieldActor = Shield;
		UE_LOG(LogNPAbility, Log, TEXT("Shield spawned for %s"), *Owner->GetName());
	}
}

void UNPShieldAbility::OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
                              FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	// Shield HP depletion is handled by the shield actor responding to damage.
	// Sync shield orientation to player camera (front-facing)
	if (SpawnedShieldActor.IsValid() && IsValid(MoverComp))
	{
		AActor* Owner = MoverComp->GetOwner();
		if (IsValid(Owner))
		{
			APawn* Pawn = Cast<APawn>(Owner);
			if (IsValid(Pawn))
			{
				const FRotator ViewRot = Pawn->GetViewRotation();
				SpawnedShieldActor->SetActorRotation(FRotator(0.f, ViewRot.Yaw, 0.f));
				SpawnedShieldActor->SetActorLocation(
					Owner->GetActorLocation() + FRotationMatrix(ViewRot).GetUnitAxis(EAxis::X) * 80.f);
			}
		}
	}

	// Check if shield HP was depleted
	if (AuxState.ShieldHP <= 0.f && AuxState.ShieldHP != 0.f)
	{
		SyncState.AbilityDurationRemainingMs = 0;
	}
}

void UNPShieldAbility::OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState)
{
	AuxState.ShieldHP = 0.f;
}

void UNPShieldAbility::OnDeactivatedAuthority(AActor* Owner)
{
	if (SpawnedShieldActor.IsValid())
	{
		SpawnedShieldActor->Destroy();
		SpawnedShieldActor = nullptr;
		UE_LOG(LogNPAbility, Log, TEXT("Shield destroyed for %s"), *Owner->GetName());
	}
}
