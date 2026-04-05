#include "Movement/NPMoverTypes.h"
#include "Stamina/NPStaminaComponent.h"

// -----------------------------------------------
// FNPMoverInputCmd
// -----------------------------------------------

FMoverDataStructBase* FNPMoverInputCmd::Clone() const
{
	return new FNPMoverInputCmd(*this);
}

bool FNPMoverInputCmd::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// Pack booleans into a single byte
	uint8 Bits = 0;
	if (Ar.IsSaving())
	{
		Bits |= bWantsToSprint ? (1 << 0) : 0;
		Bits |= bWantsToCrouch ? (1 << 1) : 0;
	}

	Ar << Bits;
	Ar << WantsToDodgeDirection;

	if (Ar.IsLoading())
	{
		bWantsToSprint = (Bits & (1 << 0)) != 0;
		bWantsToCrouch = (Bits & (1 << 1)) != 0;
	}

	bOutSuccess = true;
	return true;
}

UScriptStruct* FNPMoverInputCmd::GetScriptStruct() const
{
	return FNPMoverInputCmd::StaticStruct();
}

void FNPMoverInputCmd::ToString(FAnsiStringBuilderBase& Out) const
{
	Out.Appendf("NPInput: Sprint=%d Crouch=%d Dodge=%d", bWantsToSprint, bWantsToCrouch, WantsToDodgeDirection);
}

// -----------------------------------------------
// FNPMoverState
// -----------------------------------------------

FMoverDataStructBase* FNPMoverState::Clone() const
{
	return new FNPMoverState(*this);
}

bool FNPMoverState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << CurrentSP;
	Ar << MaxSP;
	Ar << MovementSubState;
	Ar << ModeElapsedTime;
	Ar << StaggerTimeRemaining;
	Ar << bIsCrouching;
	Ar << CapsuleHalfHeight;
	Ar << bAirDodgeUsed;
	Ar << CoyoteTimeRemaining;
	Ar << JumpHoldTimeRemaining;
	Ar << bIsJumping;
	Ar << RegenDelayRemaining;

	bOutSuccess = true;
	return true;
}

UScriptStruct* FNPMoverState::GetScriptStruct() const
{
	return FNPMoverState::StaticStruct();
}

void FNPMoverState::ToString(FAnsiStringBuilderBase& Out) const
{
	Out.Appendf("NPState: SP=%.1f/%.1f Crouch=%d Stagger=%.2f Regen=%.2f",
		CurrentSP, MaxSP, bIsCrouching, StaggerTimeRemaining, RegenDelayRemaining);
}

bool FNPMoverState::ShouldReconcile(const FMoverDataStructBase& AuthoritativeState) const
{
	const FNPMoverState& AuthState = static_cast<const FNPMoverState&>(AuthoritativeState);

	if (FMath::Abs(CurrentSP - AuthState.CurrentSP) > 1.f)
	{
		return true;
	}
	if (bIsCrouching != AuthState.bIsCrouching)
	{
		return true;
	}
	if (FMath::Abs(StaggerTimeRemaining - AuthState.StaggerTimeRemaining) > 0.05f)
	{
		return true;
	}
	if (FMath::Abs(RegenDelayRemaining - AuthState.RegenDelayRemaining) > 0.1f)
	{
		return true;
	}

	return false;
}

void FNPMoverState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FNPMoverState& FromState = static_cast<const FNPMoverState&>(From);
	const FNPMoverState& ToState = static_cast<const FNPMoverState&>(To);

	CurrentSP = FMath::Lerp(FromState.CurrentSP, ToState.CurrentSP, Pct);
	MaxSP = ToState.MaxSP;
	ModeElapsedTime = FMath::Lerp(FromState.ModeElapsedTime, ToState.ModeElapsedTime, Pct);
	StaggerTimeRemaining = FMath::Lerp(FromState.StaggerTimeRemaining, ToState.StaggerTimeRemaining, Pct);
	CapsuleHalfHeight = FMath::Lerp(FromState.CapsuleHalfHeight, ToState.CapsuleHalfHeight, Pct);
	CoyoteTimeRemaining = FMath::Lerp(FromState.CoyoteTimeRemaining, ToState.CoyoteTimeRemaining, Pct);
	JumpHoldTimeRemaining = FMath::Lerp(FromState.JumpHoldTimeRemaining, ToState.JumpHoldTimeRemaining, Pct);
	RegenDelayRemaining = FMath::Lerp(FromState.RegenDelayRemaining, ToState.RegenDelayRemaining, Pct);

	// Snap discrete state
	bIsCrouching = ToState.bIsCrouching;
	bAirDodgeUsed = ToState.bAirDodgeUsed;
	bIsJumping = ToState.bIsJumping;
	MovementSubState = ToState.MovementSubState;
}

// -----------------------------------------------
// NPStaminaUtils
// -----------------------------------------------

void NPStaminaUtils::TickSPFromComponent(FNPMoverState& State, USceneComponent* UpdatedComponent, float DeltaSeconds)
{
	float AbilityCost = 0.f;
	if (UpdatedComponent)
	{
		if (AActor* Owner = UpdatedComponent->GetOwner())
		{
			if (UNPStaminaComponent* Stamina = Owner->FindComponentByClass<UNPStaminaComponent>())
			{
				AbilityCost = Stamina->FlushPendingAbilitySPCost();
			}
		}
	}
	TickSP(State, AbilityCost, DeltaSeconds);
}
