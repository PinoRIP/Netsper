#include "Movement/NPMoverTypes.h"

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
		Bits |= bWantsSprint ? (1 << 0) : 0;
		Bits |= bWantsCrouch ? (1 << 1) : 0;
		Bits |= bWantsDodge ? (1 << 2) : 0;
		Bits |= bWantsMantle ? (1 << 3) : 0;
		Bits |= bWantsAbility ? (1 << 4) : 0;
		Bits |= bCancelAbility ? (1 << 5) : 0;
	}

	Ar << Bits;
	Ar << RequestedAbilitySlot;

	if (Ar.IsLoading())
	{
		bWantsSprint = (Bits & (1 << 0)) != 0;
		bWantsCrouch = (Bits & (1 << 1)) != 0;
		bWantsDodge = (Bits & (1 << 2)) != 0;
		bWantsMantle = (Bits & (1 << 3)) != 0;
		bWantsAbility = (Bits & (1 << 4)) != 0;
		bCancelAbility = (Bits & (1 << 5)) != 0;
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
	Out.Appendf("NPInput: Sprint=%d Crouch=%d Dodge=%d Mantle=%d",
		bWantsSprint, bWantsCrouch, bWantsDodge, bWantsMantle);
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

	bOutSuccess = true;
	return true;
}

UScriptStruct* FNPMoverState::GetScriptStruct() const
{
	return FNPMoverState::StaticStruct();
}

void FNPMoverState::ToString(FAnsiStringBuilderBase& Out) const
{
	Out.Appendf("NPState: SP=%.1f/%.1f Crouch=%d Stagger=%.2f",
		CurrentSP, MaxSP, bIsCrouching, StaggerTimeRemaining);
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

	// Snap discrete state
	bIsCrouching = ToState.bIsCrouching;
	bAirDodgeUsed = ToState.bAirDodgeUsed;
	bIsJumping = ToState.bIsJumping;
	MovementSubState = ToState.MovementSubState;
}
