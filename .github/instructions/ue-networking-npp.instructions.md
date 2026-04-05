---
description: "Use when implementing networked gameplay, replication, RPCs, Network Prediction Plugin (NPP) prediction/reconciliation, authority checks, or any multiplayer feature. Covers NPP simulation model, input commands, sync state, aux state, client prediction, server authority, and RPC patterns."
applyTo: "Source/**/*.h, Source/**/*.cpp"
---

# UE5 Networking & Network Prediction Plugin (NPP)

## Architecture Overview

Netsper uses the **Network Prediction Plugin (NPP)** for movement and ability prediction — NOT the legacy `CharacterMovementComponent` replication. NPP provides:

- **Client-side prediction**: Client simulates locally for instant responsiveness.
- **Server authority**: Server runs the same simulation authoritatively.
- **Automatic reconciliation**: NPP detects desync and rolls back/replays on the client.
- **Deterministic-ish model**: Same inputs + same state → same outputs (within floating point tolerance).

---

## NPP Core Concepts

### The Three Data Vehicles

| Struct | Direction | Purpose |
|--------|-----------|---------|
| **InputCmd** | Client → Server | Player intentions for this tick (move dir, buttons, view rotation) |
| **SyncState** | Server → Client | Authoritative state that must match (position, velocity, mode) |
| **AuxState** | Server → Client | Non-rollback state (cosmetic flags, cue triggers) |

### InputCmd — What the Player Wants

```cpp
USTRUCT()
struct FNPMoverInputCmd
{
	GENERATED_BODY()

	/** Normalized 2D movement input (forward/right). */
	FVector2D MoveInput = FVector2D::ZeroVector;

	/** Camera/view rotation at the time of input. */
	FRotator ViewRotation = FRotator::ZeroRotator;

	/** Button states — each is a single-bit bool. */
	bool bWantsJump = false;
	bool bWantsSprint = false;
	bool bWantsCrouch = false;
	bool bWantsDodge = false;
	bool bWantsAbility = false;

	/** Ability identifier if bWantsAbility is true. */
	uint8 RequestedAbilitySlot = 0;

	/** Custom NetSerialize for bandwidth optimization. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FNPMoverInputCmd> : public TStructOpsTypeTraitsBase2<FNPMoverInputCmd>
{
	enum { WithNetSerializer = true };
};
```

### SyncState — Authoritative Truth

```cpp
USTRUCT()
struct FNPMoverSyncState
{
	GENERATED_BODY()

	/** Authoritative world position. */
	FVector Location = FVector::ZeroVector;

	/** Authoritative velocity. */
	FVector Velocity = FVector::ZeroVector;

	/** Current movement mode enum. */
	ENPMovementState MovementState = ENPMovementState::Ground;

	/** Whether character is crouched. */
	bool bIsCrouching = false;

	/** Current capsule half-height (changes during crouch). */
	float CapsuleHalfHeight = 96.0f;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};
```

### AuxState — Non-Predicted Supplementary Data

```cpp
USTRUCT()
struct FNPMoverAuxState
{
	GENERATED_BODY()

	/** Cue flags — set by server, consumed by client for cosmetics. */
	bool bJustLanded = false;
	bool bJustWallrunStarted = false;
	float LastImpactVelocity = 0.0f;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};
```

---

## NPP Simulation Tick Pattern

The simulation tick is the core of NPP gameplay. It runs identically on client (prediction) and server (authority).

```cpp
void UNPGroundMovementMode::SimulationTick(
	const FSimulationTickParams& Params,
	FSimulationOutput& Output)
{
	// 1. Read input
	const FNPMoverInputCmd& Input = Params.GetTypedInputCmd<FNPMoverInputCmd>();
	FNPMoverSyncState& SyncState = Params.GetMutableTypedSyncState<FNPMoverSyncState>();
	const float DeltaSeconds = Params.DeltaSeconds;

	// 2. Compute desired velocity from input
	FVector DesiredVelocity = ComputeDesiredVelocity(Input, SyncState, DeltaSeconds);

	// 3. Apply movement physics (acceleration, friction, gravity)
	FVector NewVelocity = ApplyGroundPhysics(SyncState.Velocity, DesiredVelocity, DeltaSeconds);

	// 4. Move and sweep
	FHitResult Hit;
	FVector Delta = NewVelocity * DeltaSeconds;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	// 5. Handle collision
	if (Hit.bBlockingHit)
	{
		HandleImpact(Hit, DeltaSeconds, Delta);
		SlideAlongSurface(Delta, 1.0f - Hit.Time, Hit.Normal, Hit);
	}

	// 6. Write output state
	SyncState.Location = UpdatedComponent->GetComponentLocation();
	SyncState.Velocity = NewVelocity;

	// 7. Check mode transitions
	if (ShouldTransitionToAir(SyncState))
	{
		Output.RequestModeTransition(ENPMovementState::Air);
	}
}
```

### Critical SimulationTick Rules

1. **Pure function of inputs + state.** No reading from external mutable state (world time, random, other actors' positions) unless it's in the SyncState.
2. **No side effects on non-NPP state.** Don't modify variables that aren't in SyncState/AuxState — they won't be rolled back.
3. **Same code path for client and server.** Use `Params.IsServer()` ONLY for authority-exclusive actions (e.g., applying damage to other actors).
4. **No cosmetic logic in SimulationTick.** VFX, SFX, and animation are handled via cues or post-simulation hooks.

---

## Authority Checks

```cpp
// Check if this is the server (authority)
if (GetOwnerRole() == ROLE_Authority)
{
	// Only the server should apply damage to other actors
	TargetHealth->ApplyDamage(Damage);
}

// Check if this is the locally controlled client
if (GetOwner()->IsLocallyControlled())
{
	// Only local client should update UI
	UpdateCrosshair();
}

// Check if this is a simulated proxy (other players on our client)
if (GetOwnerRole() == ROLE_SimulatedProxy)
{
	// Smoothly interpolate simulated proxies
	SmoothProxyState(DeltaTime);
}

// Common pattern for NPP — authority vs prediction
if (Params.IsServer())
{
	// Server-only logic: validate, apply authoritative effects
}
else
{
	// Client prediction: assume success, will be corrected if wrong
}
```

---

## RPC Patterns

### Server RPCs — Client Requests Action

```cpp
/** Client requests ability activation. Server validates and executes. */
UFUNCTION(Server, Reliable, WithValidation)
void ServerRequestAbilityActivation(ENPAbilityType AbilityType);

// Implementation
void ANPCharacterPawn::ServerRequestAbilityActivation_Implementation(ENPAbilityType AbilityType)
{
	// Server validates and activates
	if (AbilityComponent->CanActivateAbility(AbilityType))
	{
		AbilityComponent->ActivateAbility(AbilityType);
	}
}

// Validation — return false to disconnect cheaters
bool ANPCharacterPawn::ServerRequestAbilityActivation_Validate(ENPAbilityType AbilityType)
{
	// Basic sanity check — is this a valid ability type?
	return AbilityType != ENPAbilityType::None;
}
```

### Client RPCs — Server Informs Client

```cpp
/** Server tells owning client their ability was denied. */
UFUNCTION(Client, Reliable)
void ClientAbilityDenied(ENPAbilityType AbilityType);

void ANPCharacterPawn::ClientAbilityDenied_Implementation(ENPAbilityType AbilityType)
{
	// Show "ability unavailable" UI feedback
	OnAbilityDenied(AbilityType);  // BlueprintImplementableEvent
}
```

### Multicast RPCs — Server Tells Everyone (Cosmetic)

```cpp
/** Play hit effect at location for all clients. */
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayHitEffect(FVector HitLocation, FVector HitNormal);

void ANPCharacterPawn::MulticastPlayHitEffect_Implementation(FVector HitLocation, FVector HitNormal)
{
	// Cosmetic only — fire BlueprintImplementableEvent for BP to handle VFX
	OnHitEffectPlayed(HitLocation, HitNormal);
}
```

### RPC Reliability Rules

| Use Case | Reliability | Why |
|----------|-------------|-----|
| Ability activation | `Reliable` | Must not be lost — affects gameplay state |
| Weapon fire confirmation | `Reliable` | Hit registration is critical |
| VFX/SFX cues | `Unreliable` | Missing a particle is acceptable |
| Chat messages | `Reliable` | Must be delivered |
| Movement correction | NPP handles this | Never manually replicate movement |
| Damage numbers | `Unreliable` | Cosmetic overlay |

---

## Property Replication

### GetLifetimeReplicatedProps

```cpp
#include "Net/UnrealNetwork.h"

void ANPCharacterPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Always replicate health
	DOREPLIFETIME(ANPCharacterPawn, CurrentHealth);

	// Only replicate to non-owners (owner knows from prediction)
	DOREPLIFETIME_CONDITION(ANPCharacterPawn, bIsSprinting, COND_SkipOwner);

	// Only replicate to owner (personal UI state)
	DOREPLIFETIME_CONDITION(ANPCharacterPawn, CurrentAmmo, COND_OwnerOnly);

	// Replicate on initial load only
	DOREPLIFETIME_CONDITION(ANPCharacterPawn, TeamId, COND_InitialOnly);
}
```

### Replication Conditions

| Condition | When to Use |
|-----------|-------------|
| `COND_None` | Always replicate (default) |
| `COND_OwnerOnly` | Only to owning client (ammo, ability charges) |
| `COND_SkipOwner` | Everyone except owner (owner predicts locally) |
| `COND_SimulatedOnly` | Only to simulated proxies |
| `COND_AutonomousOnly` | Only to autonomous proxy (owning client) |
| `COND_InitialOnly` | Once on spawn (team, player name) |

### OnRep Callbacks

```cpp
// Header
UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Health")
float CurrentHealth;

UFUNCTION()
void OnRep_CurrentHealth();

// Source
void ANPCharacterPawn::OnRep_CurrentHealth()
{
	// Called on clients when server updates CurrentHealth
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		OnPlayerDied.Broadcast();
	}
}
```

---

## NPP-Specific Networking Rules

1. **Never manually replicate position/velocity.** NPP SyncState handles this. Setting `bReplicates = true` and `SetReplicatingMovement(false)` is correct.

2. **Stamina is part of NPP simulation.** Stamina consumption happens in SimulationTick and is stored in SyncState so it's predicted and reconciled.

3. **Ability state uses a separate NPP simulation.** `UNPAbilityComponent` registers its own simulation with NPP, with its own InputCmd/SyncState/AuxState.

4. **Cues for cosmetics.** NPP provides a cue system — use it for one-shot events (jumped, landed, started wallrun). Don't replicate cosmetic booleans.

5. **Bandwidth budget.** Competitive FPS needs tight bandwidth. Use `NetSerialize` to bit-pack structs. Quantize floats. Use smallest possible types.

```cpp
// Example: efficient NetSerialize
bool FNPMoverInputCmd::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// Pack move input as two int16 (-10000 to 10000)
	int16 MoveX = FMath::Clamp(FMath::RoundToInt(MoveInput.X * 10000.0f), -10000, 10000);
	int16 MoveY = FMath::Clamp(FMath::RoundToInt(MoveInput.Y * 10000.0f), -10000, 10000);
	Ar << MoveX << MoveY;

	if (Ar.IsLoading())
	{
		MoveInput.X = MoveX / 10000.0f;
		MoveInput.Y = MoveY / 10000.0f;
	}

	// Pack rotation
	ViewRotation.SerializeCompressedShort(Ar);

	// Pack booleans into a single byte
	uint8 ButtonFlags = 0;
	if (Ar.IsSaving())
	{
		ButtonFlags |= bWantsJump   ? (1 << 0) : 0;
		ButtonFlags |= bWantsSprint ? (1 << 1) : 0;
		ButtonFlags |= bWantsCrouch ? (1 << 2) : 0;
		ButtonFlags |= bWantsDodge  ? (1 << 3) : 0;
		ButtonFlags |= bWantsAbility? (1 << 4) : 0;
	}
	Ar << ButtonFlags;
	if (Ar.IsLoading())
	{
		bWantsJump    = (ButtonFlags & (1 << 0)) != 0;
		bWantsSprint  = (ButtonFlags & (1 << 1)) != 0;
		bWantsCrouch  = (ButtonFlags & (1 << 2)) != 0;
		bWantsDodge   = (ButtonFlags & (1 << 3)) != 0;
		bWantsAbility = (ButtonFlags & (1 << 4)) != 0;
	}

	if (bWantsAbility)
	{
		Ar << RequestedAbilitySlot;
	}

	bOutSuccess = true;
	return true;
}
```

---

## Common Networking Mistakes to Avoid

1. **Don't read `GetWorld()->GetTimeSeconds()` in SimulationTick.** It differs between client and server. Use `Params.DeltaSeconds` for time advancement.

2. **Don't spawn actors in SimulationTick.** Actor spawning is non-deterministic. Queue spawn requests and handle them in a post-tick authority callback.

3. **Don't access other actors' state in SimulationTick.** Other actors' positions are not rolled back during reconciliation. Only read from your own SyncState.

4. **Don't use `FMath::RandRange` in SimulationTick.** Random numbers diverge between client and server. If you need randomness, seed it from SyncState.

5. **Don't forget `WithValidation` on Server RPCs.** Every `UFUNCTION(Server, Reliable)` should have `WithValidation` to protect against malformed/malicious input.

6. **Don't replicate what NPP already handles.** Position, velocity, and movement state are in SyncState — don't also put them in `DOREPLIFETIME`.
