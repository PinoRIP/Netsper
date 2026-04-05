#include "Abilities/NPAbilityComponent.h"
#include "Abilities/NPAbilityBase.h"
#include "MoverComponent.h"
#include "Stamina/NPStaminaComponent.h"
#include "Net/UnrealNetwork.h"
#include "Netsper.h"

UNPAbilityComponent::UNPAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UNPAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		CachedMoverComponent = Owner->FindComponentByClass<UMoverComponent>();
		CachedStaminaComponent = Owner->FindComponentByClass<UNPStaminaComponent>();
	}

	// Ensure we tick before MoverComponent so layered moves are queued before consumption
	if (IsValid(CachedMoverComponent))
	{
		AddTickPrerequisiteComponent(CachedMoverComponent);
	}
}

void UNPAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const int32 DeltaMs = FMath::RoundToInt32(DeltaTime * 1000.f);
	SimulationTick(DeltaMs);

	// Clear one-shot input
	PendingInput.ActivationSlot = 0;
	PendingInput.bCancelActive = false;
}

void UNPAbilityComponent::SimulationTick(int32 DeltaMs)
{
	// 1. Tick cooldown
	if (AbilityState.AbilityCooldownRemainingMs > 0)
	{
		AbilityState.AbilityCooldownRemainingMs = FMath::Max(0, AbilityState.AbilityCooldownRemainingMs - DeltaMs);
	}

	// 2. Tick active duration
	if (AbilityState.bAbilityActive && AbilityState.AbilityDurationRemainingMs > 0)
	{
		AbilityState.AbilityDurationRemainingMs = FMath::Max(0, AbilityState.AbilityDurationRemainingMs - DeltaMs);

		if (AbilityState.AbilityDurationRemainingMs == 0)
		{
			DeactivateAbility();
		}
	}

	// 3. Handle cancel request
	if (PendingInput.bCancelActive && AbilityState.bAbilityActive)
	{
		DeactivateAbility();
	}

	// 4. Handle activation request
	if (PendingInput.ActivationSlot > 0 && !AbilityState.bAbilityActive && AbilityState.AbilityCooldownRemainingMs == 0)
	{
		ActivateAbility(PendingInput);
	}

	// 5. Tick active ability
	if (AbilityState.bAbilityActive && IsValid(EquippedAbility))
	{
		EquippedAbility->OnTick(DeltaMs, AbilityState, AuxState, CachedMoverComponent);
	}
}

void UNPAbilityComponent::ActivateAbility(const FNPAbilityInputCmd& InputCmd)
{
	if (!IsValid(EquippedAbility))
	{
		return;
	}

	// Validate SP cost
	const float SPCost = EquippedAbility->GetSPCost();
	if (SPCost > 0.f && IsValid(CachedStaminaComponent))
	{
		if (!CachedStaminaComponent->TryConsumeSP(SPCost))
		{
			UE_LOG(LogNPAbility, Verbose, TEXT("Ability activation failed: insufficient SP (need %.1f)"), SPCost);
			return;
		}
	}

	AbilityState.bAbilityActive = true;
	AbilityState.ActiveAbilityTag = EquippedAbility->GetAbilityTag();

	const int32 DurationMs = EquippedAbility->GetDurationMs();
	AbilityState.AbilityDurationRemainingMs = DurationMs;
	AbilityState.AbilityCooldownRemainingMs = 0;

	EquippedAbility->OnActivated(InputCmd, AbilityState, AuxState, CachedMoverComponent);

	// Server-only: spawn world actors
	if (GetOwner()->HasAuthority())
	{
		EquippedAbility->OnActivatedAuthority(GetOwner(), InputCmd);
	}

	OnAbilityActivated.Broadcast(AbilityState.ActiveAbilityTag);
	UE_LOG(LogNPAbility, Log, TEXT("Ability activated: %s"), *AbilityState.ActiveAbilityTag.ToString());
}

void UNPAbilityComponent::DeactivateAbility()
{
	if (!AbilityState.bAbilityActive)
	{
		return;
	}

	const FGameplayTag DeactivatedTag = AbilityState.ActiveAbilityTag;

	if (IsValid(EquippedAbility))
	{
		EquippedAbility->OnDeactivated(AbilityState, AuxState);

		// Server-only: destroy world actors
		if (GetOwner()->HasAuthority())
		{
			EquippedAbility->OnDeactivatedAuthority(GetOwner());
		}

		// Set cooldown
		AbilityState.AbilityCooldownRemainingMs = EquippedAbility->GetCooldownMs();
	}

	AbilityState.bAbilityActive = false;
	AbilityState.ActiveAbilityTag = FGameplayTag();
	AbilityState.AbilityDurationRemainingMs = 0;

	OnAbilityDeactivated.Broadcast(DeactivatedTag);
	UE_LOG(LogNPAbility, Log, TEXT("Ability deactivated: %s"), *DeactivatedTag.ToString());
}

void UNPAbilityComponent::EquipAbility(TSubclassOf<UNPAbilityBase> AbilityClass)
{
	if (!AbilityClass)
	{
		return;
	}

	// Deactivate current if active
	if (AbilityState.bAbilityActive)
	{
		DeactivateAbility();
	}

	EquippedAbility = NewObject<UNPAbilityBase>(this, AbilityClass);
	UE_LOG(LogNPAbility, Log, TEXT("Equipped ability: %s"), *AbilityClass->GetName());
}

void UNPAbilityComponent::RequestActivation(uint8 Slot)
{
	PendingInput.ActivationSlot = Slot;
}

void UNPAbilityComponent::RequestCancel()
{
	PendingInput.bCancelActive = true;
}

void UNPAbilityComponent::OnRep_AbilityState()
{
}

void UNPAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNPAbilityComponent, AbilityState);
}
