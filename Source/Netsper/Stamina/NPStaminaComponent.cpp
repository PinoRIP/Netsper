#include "Stamina/NPStaminaComponent.h"
#include "Net/UnrealNetwork.h"
#include "Netsper.h"

UNPStaminaComponent::UNPStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	CurrentSP = MaxSP;
}

void UNPStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentSP = MaxSP;
}

void UNPStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Regen logic (server-authoritative, also runs locally for responsiveness)
	if (RegenDelayRemaining > 0.f)
	{
		RegenDelayRemaining -= DeltaTime;
		return;
	}

	if (CurrentSP < MaxSP)
	{
		float RegenDelta = RegenRate * DeltaTime;
		// TODO(NP): Apply SprintRegenPenalty and CombatRegenPenalty based on active state

		const float OldSP = CurrentSP;
		CurrentSP = FMath::Min(CurrentSP + RegenDelta, MaxSP);

		if (!FMath::IsNearlyEqual(OldSP, CurrentSP))
		{
			OnStaminaChanged.Broadcast(CurrentSP, MaxSP);
		}
	}
}

void UNPStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNPStaminaComponent, CurrentSP);
}

float UNPStaminaComponent::GetCurrentSP() const
{
	return CurrentSP;
}

float UNPStaminaComponent::GetMaxSP() const
{
	return MaxSP;
}

bool UNPStaminaComponent::TryConsumeSP(float Amount)
{
	if (CurrentSP >= Amount)
	{
		ConsumeSP(Amount);
		return true;
	}
	return false;
}

void UNPStaminaComponent::ConsumeSP(float Amount)
{
	const float OldSP = CurrentSP;
	CurrentSP = FMath::Max(0.f, CurrentSP - Amount);
	RegenDelayRemaining = RegenDelay;

	if (!FMath::IsNearlyEqual(OldSP, CurrentSP))
	{
		OnStaminaChanged.Broadcast(CurrentSP, MaxSP);
	}
}

void UNPStaminaComponent::RestoreSP(float Amount)
{
	const float OldSP = CurrentSP;
	CurrentSP = FMath::Min(CurrentSP + Amount, MaxSP);

	if (!FMath::IsNearlyEqual(OldSP, CurrentSP))
	{
		OnStaminaChanged.Broadcast(CurrentSP, MaxSP);
	}
}

float UNPStaminaComponent::GetStaminaPercent() const
{
	return MaxSP > 0.f ? CurrentSP / MaxSP : 0.f;
}

void UNPStaminaComponent::NotifyConsumption()
{
	RegenDelayRemaining = RegenDelay;
}

void UNPStaminaComponent::OnRep_CurrentSP()
{
	OnStaminaChanged.Broadcast(CurrentSP, MaxSP);
}
