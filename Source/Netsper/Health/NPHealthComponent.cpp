#include "Health/NPHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Netsper.h"

UNPHealthComponent::UNPHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	CurrentHealth = MaxHealth;
}

void UNPHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

void UNPHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPHealthComponent, CurrentHealth);
}

void UNPHealthComponent::ApplyDamage(float Amount, AActor* Instigator, FGameplayTag DamageType)
{
	// Damage is server-authoritative only
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!IsAlive())
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, MaxHealth);

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}

	if (CurrentHealth <= 0.f)
	{
		OnPlayerDied.Broadcast(Instigator);
	}
}

void UNPHealthComponent::ApplyHeal(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!IsAlive())
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}
}

bool UNPHealthComponent::IsAlive() const
{
	return CurrentHealth > 0.f;
}

float UNPHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f;
}

void UNPHealthComponent::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}
