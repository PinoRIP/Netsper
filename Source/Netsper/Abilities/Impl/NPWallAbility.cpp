#include "Abilities/Impl/NPWallAbility.h"
#include "Abilities/NPAbilityTypes.h"
#include "MoverComponent.h"
#include "Netsper.h"

FGameplayTag UNPWallAbility::GetAbilityTag() const
{
	return FGameplayTag::RequestGameplayTag(FName("Ability.Wall"));
}

void UNPWallAbility::OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd& InputCmd)
{
	if (!IsValid(Owner) || !WallActorClass)
	{
		return;
	}

	// Trace from player feet forward to find floor surface
	const FVector TraceStart = Owner->GetActorLocation() + Owner->GetActorForwardVector() * PlacementDistance;
	const FVector TraceEnd = TraceStart - FVector::UpVector * 500.f;

	FHitResult FloorHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	FVector SpawnLocation = TraceStart;
	FRotator SpawnRotation = FRotator(0.f, Owner->GetActorRotation().Yaw, 0.f);

	if (Owner->GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		SpawnLocation = FloorHit.ImpactPoint;
		// Align wall to floor normal
		SpawnRotation = FRotationMatrix::MakeFromZX(FloorHit.ImpactNormal, Owner->GetActorForwardVector()).Rotator();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Wall = Owner->GetWorld()->SpawnActor<AActor>(WallActorClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (IsValid(Wall))
	{
		SpawnedWallActor = Wall;
		UE_LOG(LogNPAbility, Log, TEXT("Wall spawned at %s for %s"), *SpawnLocation.ToString(), *Owner->GetName());
	}
}

void UNPWallAbility::OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState)
{
	AuxState.WallHP = 0.f;
}

void UNPWallAbility::OnDeactivatedAuthority(AActor* Owner)
{
	if (SpawnedWallActor.IsValid())
	{
		SpawnedWallActor->Destroy();
		SpawnedWallActor = nullptr;
		UE_LOG(LogNPAbility, Log, TEXT("Wall destroyed for %s"), *Owner->GetName());
	}
}
