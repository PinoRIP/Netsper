#include "Stamina/NPStaminaComponent.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "Movement/NPMoverTypes.h"
#include "Netsper.h"

UNPStaminaComponent::UNPStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		CachedMoverComponent = Owner->FindComponentByClass<UMoverComponent>();
	}

	// Bind to Mover's post-simulation tick to broadcast SP changes for UI
	if (CachedMoverComponent)
	{
		CachedMoverComponent->OnPostSimulationTick.AddDynamic(this, &UNPStaminaComponent::OnMoverPostSimulationTick);
	}

	LastBroadcastSP = MaxSP;
}

float UNPStaminaComponent::GetSPFromMoverState() const
{
	if (CachedMoverComponent)
	{
		const FMoverSyncState& SyncState = CachedMoverComponent->GetSyncState();
		const FNPMoverState* NPState = SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();
		if (NPState)
		{
			return NPState->CurrentSP;
		}
	}
	return MaxSP;
}

float UNPStaminaComponent::GetCurrentSP() const
{
	// Account for pending ability cost not yet applied by the Mover sim
	return FMath::Max(0.f, GetSPFromMoverState() - PendingAbilitySPCost);
}

float UNPStaminaComponent::GetMaxSP() const
{
	if (CachedMoverComponent)
	{
		const FMoverSyncState& SyncState = CachedMoverComponent->GetSyncState();
		const FNPMoverState* NPState = SyncState.SyncStateCollection.FindDataByType<FNPMoverState>();
		if (NPState)
		{
			return NPState->MaxSP;
		}
	}
	return MaxSP;
}

bool UNPStaminaComponent::TryConsumeSP(float Amount)
{
	if (GetCurrentSP() >= Amount)
	{
		ConsumeSP(Amount);
		return true;
	}
	return false;
}

void UNPStaminaComponent::ConsumeSP(float Amount)
{
	PendingAbilitySPCost += Amount;

	// Fire delegate immediately for responsive UI
	const float EffectiveSP = GetCurrentSP();
	OnStaminaChanged.Broadcast(EffectiveSP, GetMaxSP());
}

void UNPStaminaComponent::RestoreSP(float Amount)
{
	// Negative cost = restore
	PendingAbilitySPCost -= Amount;

	const float EffectiveSP = GetCurrentSP();
	OnStaminaChanged.Broadcast(EffectiveSP, GetMaxSP());
}

float UNPStaminaComponent::GetStaminaPercent() const
{
	const float Max = GetMaxSP();
	return Max > 0.f ? GetCurrentSP() / Max : 0.f;
}

float UNPStaminaComponent::FlushPendingAbilitySPCost()
{
	const float Cost = FMath::Max(0.f, PendingAbilitySPCost);
	PendingAbilitySPCost = 0.f;
	return Cost;
}

void UNPStaminaComponent::OnMoverPostSimulationTick(const FMoverTimeStep& TimeStep)
{
	const float CurrentSP = GetCurrentSP();
	if (!FMath::IsNearlyEqual(CurrentSP, LastBroadcastSP, 0.1f))
	{
		LastBroadcastSP = CurrentSP;
		OnStaminaChanged.Broadcast(CurrentSP, GetMaxSP());
	}
}
