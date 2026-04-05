#include "Weapons/NPWeaponComponent.h"
#include "Weapons/NPWeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "Netsper.h"

UNPWeaponComponent::UNPWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UNPWeaponBase* UNPWeaponComponent::GetActiveWeapon() const
{
	if (WeaponSlots.IsValidIndex(ActiveWeaponIndex))
	{
		return WeaponSlots[ActiveWeaponIndex];
	}
	return nullptr;
}

void UNPWeaponComponent::StartFire()
{
	UNPWeaponBase* Weapon = GetActiveWeapon();
	if (IsValid(Weapon) && Weapon->CanFire())
	{
		AActor* Owner = GetOwner();
		FVector Origin = IsValid(Owner) ? Owner->GetActorLocation() : FVector::ZeroVector;
		FRotator Direction = IsValid(Owner) ? Owner->GetActorRotation() : FRotator::ZeroRotator;

		Weapon->StartFire(Origin, Direction);

		// Send to server for validation
		Server_Fire(Origin, Direction, GetWorld()->GetTimeSeconds());
	}
}

void UNPWeaponComponent::StopFire()
{
	UNPWeaponBase* Weapon = GetActiveWeapon();
	if (IsValid(Weapon))
	{
		Weapon->StopFire();
	}
}

void UNPWeaponComponent::StartAltFire()
{
	UNPWeaponBase* Weapon = GetActiveWeapon();
	if (IsValid(Weapon))
	{
		Weapon->StartAltFire();
	}
}

void UNPWeaponComponent::Reload()
{
	UNPWeaponBase* Weapon = GetActiveWeapon();
	if (IsValid(Weapon) && Weapon->bIsReloading == false)
	{
		Weapon->Reload();
	}
}

void UNPWeaponComponent::SwitchWeapon(int32 Direction)
{
	if (WeaponSlots.Num() <= 1)
	{
		return;
	}

	int32 NewIndex = (ActiveWeaponIndex + Direction) % WeaponSlots.Num();
	if (NewIndex < 0)
	{
		NewIndex += WeaponSlots.Num();
	}

	if (NewIndex != ActiveWeaponIndex)
	{
		UNPWeaponBase* OldWeapon = GetActiveWeapon();
		if (IsValid(OldWeapon))
		{
			OldWeapon->OnUnequipped();
		}

		ActiveWeaponIndex = NewIndex;

		UNPWeaponBase* NewWeapon = GetActiveWeapon();
		if (IsValid(NewWeapon))
		{
			NewWeapon->OnEquipped();
		}

		Server_SwitchWeapon(NewIndex);
		OnWeaponSwitched.Broadcast(NewIndex, NewWeapon);
	}
}

void UNPWeaponComponent::Server_SwitchWeapon_Implementation(int32 NewIndex)
{
	if (!WeaponSlots.IsValidIndex(NewIndex))
	{
		UE_LOG(LogNP, Warning, TEXT("Server_SwitchWeapon: Invalid index %d"), NewIndex);
		return;
	}

	if (NewIndex != ActiveWeaponIndex)
	{
		UNPWeaponBase* OldWeapon = GetActiveWeapon();
		if (IsValid(OldWeapon))
		{
			OldWeapon->OnUnequipped();
		}

		ActiveWeaponIndex = NewIndex;

		UNPWeaponBase* NewWeapon = GetActiveWeapon();
		if (IsValid(NewWeapon))
		{
			NewWeapon->OnEquipped();
		}
	}
}

void UNPWeaponComponent::Server_Fire_Implementation(FVector Origin, FRotator Direction, float Timestamp)
{
	UNPWeaponBase* Weapon = GetActiveWeapon();
	if (!IsValid(Weapon))
	{
		UE_LOG(LogNP, Warning, TEXT("Server_Fire: No active weapon"));
		return;
	}

	if (!Weapon->CanFire())
	{
		UE_LOG(LogNP, Warning, TEXT("Server_Fire: Weapon cannot fire (ammo/reload)"));
		return;
	}

	// Server-authoritative fire + damage application
	Weapon->StartFire(Origin, Direction);
}

void UNPWeaponComponent::OnRep_Weapons()
{
}

void UNPWeaponComponent::OnRep_ActiveWeaponIndex()
{
	OnWeaponSwitched.Broadcast(ActiveWeaponIndex, GetActiveWeapon());
}

void UNPWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNPWeaponComponent, WeaponSlots);
	DOREPLIFETIME(UNPWeaponComponent, ActiveWeaponIndex);
}
