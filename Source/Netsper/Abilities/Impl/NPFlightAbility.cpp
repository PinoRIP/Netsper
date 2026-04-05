#include "Abilities/Impl/NPFlightAbility.h"
#include "Abilities/NPAbilityTypes.h"
#include "MoverComponent.h"
#include "Movement/LayeredMoves/NPFlightLayeredMove.h"
#include "Stamina/NPStaminaComponent.h"
#include "Netsper.h"

FGameplayTag UNPFlightAbility::GetAbilityTag() const
{
	return FGameplayTag::RequestGameplayTag(FName("Ability.Flight"));
}

void UNPFlightAbility::OnActivated(const FNPAbilityInputCmd& InputCmd, FNPAbilitySyncState& SyncState,
                                   FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	bFuelDepleted = false;

	// Queue flight layered move (persistent — stays until removed)
	if (IsValid(MoverComp))
	{
		TSharedPtr<FNPFlightLayeredMove> FlightMove = MakeShared<FNPFlightLayeredMove>();
		FlightMove->DurationMs = 0.f; // Infinite
		MoverComp->QueueLayeredMove(FlightMove);
	}

	UE_LOG(LogNPAbility, Log, TEXT("Flight activated"));
}

void UNPFlightAbility::OnTick(int32 DeltaMs, FNPAbilitySyncState& SyncState,
                              FNPAbilityAuxState& AuxState, UMoverComponent* MoverComp)
{
	const float DeltaSeconds = DeltaMs / 1000.f;

	// Drain SP
	AActor* Owner = MoverComp ? MoverComp->GetOwner() : nullptr;
	UNPStaminaComponent* StaminaComp = IsValid(Owner) ? Owner->FindComponentByClass<UNPStaminaComponent>() : nullptr;

	if (IsValid(StaminaComp))
	{
		const float DrainAmount = FlightDrainRate * DeltaSeconds;
		StaminaComp->ConsumeSP(DrainAmount);

		if (StaminaComp->GetCurrentSP() <= 0.f)
		{
			bFuelDepleted = true;

			// Force deactivation — signal to ability component via duration
			SyncState.AbilityDurationRemainingMs = 0;
		}
	}

	// Re-queue flight layered move each tick to keep it active
	if (IsValid(MoverComp) && SyncState.AbilityDurationRemainingMs != 0)
	{
		TSharedPtr<FNPFlightLayeredMove> FlightMove = MakeShared<FNPFlightLayeredMove>();
		FlightMove->DurationMs = static_cast<float>(DeltaMs); // Single tick duration
		MoverComp->QueueLayeredMove(FlightMove);
	}
}

void UNPFlightAbility::OnDeactivated(FNPAbilitySyncState& SyncState, FNPAbilityAuxState& AuxState)
{
	// Only set cooldown if fuel was depleted, not on manual cancel
	if (!bFuelDepleted)
	{
		SyncState.AbilityCooldownRemainingMs = 0;
	}

	bFuelDepleted = false;
	UE_LOG(LogNPAbility, Log, TEXT("Flight deactivated (fuel depleted: %s)"), bFuelDepleted ? TEXT("true") : TEXT("false"));
}
