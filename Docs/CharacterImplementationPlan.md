# Character Implementation Plan — Novector / Netsper
## UE 5.7 · Mover 2.0 · Network Prediction Plugin · Component Architecture

---

## Table of Contents

1. [Overview & Goals](#1-overview--goals)
2. [Project Configuration](#2-project-configuration)
3. [Architecture Overview](#3-architecture-overview)
4. [Source File Structure](#4-source-file-structure)
5. [Phase 1 — Core Pawn](#5-phase-1--core-pawn)
6. [Phase 2 — Input System](#6-phase-2--input-system)
7. [Phase 3 — Movement System (Mover Modes)](#7-phase-3--movement-system-mover-modes)
8. [Phase 4 — Stamina Component (SP)](#8-phase-4--stamina-component-sp)
9. [Phase 5 — Custom Ability System (NPP)](#9-phase-5--custom-ability-system-npp)
10. [Phase 6 — Health Component](#10-phase-6--health-component)
11. [Phase 7 — Weapon Component](#11-phase-7--weapon-component)
12. [Phase 8 — Camera Component](#12-phase-8--camera-component)
13. [Cross-System Integration](#13-cross-system-integration)
14. [Networking Topology](#14-networking-topology)
15. [Implementation Order](#15-implementation-order)

---

## 1. Overview & Goals

### Design Target (from GDD)

A high-velocity first-person arena shooter where movement mastery defines skill.
The character pawn must support:

| Category        | Mechanics |
|----------------|-----------|
| Ground movement | Walk, Sprint (SP), Crouch |
| Airborne        | Jump (apex-inspired), Coyote time, Air control |
| Advanced        | Slide, Wallrun, Wall-jump, Wall-climb, Mantle (normal & SP-boosted) |
| Reactive        | Dodge (ground + air, SP), Landing stagger, Landing roll |
| Resource        | SP (Stamina Points) — regenerating, rate varies by state |
| Abilities       | Grapple, Shield, Wall, Flight, Invisibility |
| Combat          | Ranged weapons, melee weapons, 1 equipped ability |
| View            | True first-person, camera reacts to speed / impact / tilt |

### Technical Goals

- `APawn` base (no ACharacter or CMC)
- `UMoverComponent` (Mover 2.0) as the movement driver
- Network Prediction Plugin (NPP) as the Mover backend for movement prediction
- Dedicated NPP simulation for the ability system (separate from Mover's simulation)
- Every feature is an independent, swappable component
- No Gameplay Ability System (GAS); all ability logic is custom-built

---

## 2. Project Configuration

### 2.1 Plugins to Enable (`Netsper.uproject`)

```json
{
  "Plugins": [
    { "Name": "ModelingToolsEditorMode", "Enabled": true, "TargetAllowList": ["Editor"] },
    { "Name": "Mover",                   "Enabled": true },
    { "Name": "NetworkPrediction",        "Enabled": true },
    { "Name": "EnhancedInput",            "Enabled": true },
    { "Name": "GameplayTags",             "Enabled": true }
  ]
}
```

> **Note:** `GameplayTags` is used only for lightweight event tagging on abilities/effects — no GAS classes are referenced.

### 2.2 Module Dependencies (`Netsper.Build.cs`)

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine", "InputCore",
    "EnhancedInput",
    "Mover",              // UMoverComponent and all movement modes
    "MoverExamples",      // Optional: reference implementations to study
    "NetworkPrediction",  // NPP base types for ability simulation
    "NetCore",            // FNetBitWriter, net serialisation helpers
    "GameplayTags"        // FGameplayTag for ability/effect identification
});

PrivateDependencyModuleNames.AddRange(new string[]
{
    "PhysicsCore"         // Needed for sweep/overlap queries in movement modes
});
```

### 2.3 DefaultEngine.ini Additions

```ini
[/Script/Engine.GameNetworkManager]
TotalNetBandwidth=32000
MaxDynamicBandwidth=8000
MinDynamicBandwidth=4000

[NetworkPrediction]
; Ensure NPP state history depth matches the expected RTT budget
MaxSimulationFrames=128

[Mover]
; Mover uses NPP backend by default in 5.7
bUseNetworkPredictionBackend=true
```

---

## 3. Architecture Overview

### 3.1 Component Map

```
ANPCharacterPawn (APawn)
│
├── UCapsuleComponent                     ← Root, physics collision
├── USkeletalMeshComponent                ← Full-body mesh (third-person / shadow)
├── USkeletalMeshComponent (FirstPerson)  ← Arms-only mesh visible in FP view
│
├── USpringArmComponent                   ← Camera boom
├── UCameraComponent                      ← Player view
│
├── UMoverComponent                       ← Movement simulation driver (NPP backend)
│   ├── UNPGroundMovementMode             ← Walk / Sprint / Crouch
│   ├── UNPAirMovementMode                ← Falling, air control
│   ├── UNPSlideMode                      ← Momentum slide
│   ├── UNPWallRunMode                    ← Side-wall traversal
│   ├── UNPWallClimbMode                  ← Short vertical climb
│   └── UNPMantleMode                     ← Ledge mantle
│
├── UNPMovementInputComponent             ← Translates Enhanced Input → MoverInputCmd
├── UNPStaminaComponent                   ← SP resource, NPP-aware tick
├── UNPAbilityComponent                   ← Custom ability system, own NPP simulation
├── UNPHealthComponent                    ← HP, damage, death (standard replication)
├── UNPWeaponComponent                    ← Weapon slot management
└── UNPCameraComponent                    ← Camera FX (tilt, FOV, shake)
```

### 3.2 Data-Flow per Tick

```
Enhanced Input
       │
       ▼
UNPMovementInputComponent
  · ProduceInput() fills FNPMoverInputCmd
  · Queues ability activations → UNPAbilityComponent
       │
       ▼
UMoverComponent (NPP Backend)
  · Client predicts locally
  · Server simulates authoritatively
  · Server sends corrections → client reconciles
       │
       ├── Reads FNPMoverInputCmd each tick
       ├── Queries UNPStaminaComponent via blackboard token
       └── Dispatches to active UBaseMovementMode subclass
               │
               └── SimulationTick() → FMoverTickEndData
                     (position, velocity, orientation, mode transitions)

UNPAbilityComponent (separate NPP sim)
  · Client predicts ability activation / cooldown
  · Server authoritative for cooldown enforcement
  · Ability effects that touch movement inject FLayeredMove into UMoverComponent
```

### 3.3 Key Design Principles

- **Components own their state.** No monolithic character class holding all variables.
- **Interfaces decouple systems.** Movement modes query SP through `INPStaminaProvider`, not a hard reference.
- **Layered Moves bridge systems.** Abilities that affect movement (Grapple, Flight) inject `FLayeredMoveBase` subclasses into `UMoverComponent` rather than directly setting velocity.
- **All predicted state is in structs.** NPP and Mover require state to be in registered structs — no bare UPROPERTY on components for anything that is predicted.
- **Cues carry events.** Landing stagger, ability activations, wall-run start/end are broadcast as NPP Cues so VFX/SFX/animation can react without polling.

---

## 4. Source File Structure

```
Source/Netsper/
├── Netsper.Build.cs
├── Netsper.h / Netsper.cpp
│
├── Character/
│   ├── NPCharacterPawn.h
│   └── NPCharacterPawn.cpp
│
├── Input/
│   ├── NPMovementInputComponent.h
│   └── NPMovementInputComponent.cpp
│
├── Movement/
│   ├── NPMoverTypes.h                   ← Shared structs: FNPMoverInputCmd, FNPMoverSyncState
│   │
│   ├── Modes/
│   │   ├── NPGroundMovementMode.h / .cpp
│   │   ├── NPAirMovementMode.h    / .cpp
│   │   ├── NPSlideMode.h          / .cpp
│   │   ├── NPWallRunMode.h        / .cpp
│   │   ├── NPWallClimbMode.h      / .cpp
│   │   └── NPMantleMode.h         / .cpp
│   │
│   └── LayeredMoves/
│       ├── NPDodgeLayeredMove.h   / .cpp
│       ├── NPJumpLayeredMove.h    / .cpp
│       └── NPLandingRollLayeredMove.h / .cpp
│
├── Stamina/
│   ├── NPStaminaComponent.h       / .cpp
│   ├── NPStaminaTypes.h           ← FNPStaminaSyncState
│   └── NPStaminaInterface.h       ← INPStaminaProvider (pure interface)
│
├── Abilities/
│   ├── NPAbilityComponent.h       / .cpp
│   ├── NPAbilityTypes.h           ← FAbilityInputCmd, FAbilitySyncState, FAbilityAuxState
│   ├── NPAbilityBase.h            / .cpp
│   └── Impl/
│       ├── NPGrappleAbility.h     / .cpp
│       ├── NPShieldAbility.h      / .cpp
│       ├── NPWallAbility.h        / .cpp
│       ├── NPFlightAbility.h      / .cpp
│       └── NPInvisibilityAbility.h/ .cpp
│
├── Health/
│   ├── NPHealthComponent.h        / .cpp
│   └── NPDamageInterface.h        ← INPDamageable
│
├── Weapons/
│   ├── NPWeaponComponent.h        / .cpp
│   └── NPWeaponBase.h             / .cpp
│
└── Camera/
    ├── NPCameraComponent.h        / .cpp
    └── NPCameraTypes.h            ← FCameraShakeParams, etc.
```

---

## 5. Phase 1 — Core Pawn

### 5.1 `ANPCharacterPawn`

**Inherits:** `APawn`, `IMoverInputProducerInterface`

The pawn is intentionally thin. It owns the component hierarchy and wires them together; all logic lives in components.

#### Key Responsibilities
- Create and attach all components at construction
- Implement `ProduceInput(int32 SimTimeMs, FMoverInputCmdContext&)` — delegates to `UNPMovementInputComponent`
- Expose `GetMoverComponent()`, `GetStaminaComponent()`, etc. for Blueprint access
- Handle `SetupPlayerInputComponent` — delegates to `UNPMovementInputComponent`
- Manage `GetPawnViewLocation()` / `GetViewRotation()` for correct camera transforms

#### Member Layout

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision")
TObjectPtr<UCapsuleComponent> CapsuleComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
TObjectPtr<USkeletalMeshComponent> BodyMesh;          // full body, hidden in FP

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
TObjectPtr<USkeletalMeshComponent> ArmsMesh;          // FP arms, only visible locally

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
TObjectPtr<USpringArmComponent> CameraBoom;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
TObjectPtr<UCameraComponent> Camera;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
TObjectPtr<UMoverComponent> MoverComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
TObjectPtr<UNPMovementInputComponent> MovementInputComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gameplay")
TObjectPtr<UNPStaminaComponent> StaminaComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gameplay")
TObjectPtr<UNPAbilityComponent> AbilityComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gameplay")
TObjectPtr<UNPHealthComponent> HealthComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
TObjectPtr<UNPWeaponComponent> WeaponComponent;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
TObjectPtr<UNPCameraComponent> CameraEffectsComponent;
```

#### Capsule & Mesh Setup

```
Capsule:  96 cm tall, 42 cm radius  (tuned for competitive FPS, similar to Apex)
BodyMesh: offset -48 cm Z, facing forward (+X)
ArmsMesh: attached to Camera, no offset (follows view exactly)
CameraBoom: zero arm length, attached to capsule at eye-height offset (+80 cm Z)
Camera: attached to boom end; no lag (competitive FPS requires zero lag)
```

#### Crouch Height Change

Capsule height reduced from 96 to 60 cm when crouching. The mesh offset adjusts in `UNPGroundMovementMode::SimulationTick` to keep feet planted. Height change is stored in the Mover sync state (not a raw UPROPERTY) to ensure network prediction.

---

## 6. Phase 2 — Input System

### 6.1 `UNPMovementInputComponent`

**Inherits:** `UActorComponent`

Bridges the Enhanced Input system to Mover's `FNPMoverInputCmd` each simulation tick.

#### Rationale for a Dedicated Input Component

Mover requires input to be produced inside `ProduceInput()`, which is called from the Mover simulation tick (not the normal actor tick). A dedicated component isolates all input-to-movement translation in one place and makes it independently testable.

#### Input Actions (Data Assets)

| Action | Type | Description |
|--------|------|-------------|
| `IA_Move` | `FVector2D` | WASD / left stick |
| `IA_Look` | `FVector2D` | Mouse / right stick |
| `IA_Jump` | `bool` (triggered/completed) | Space / face button |
| `IA_Sprint` | `bool` (held) | Shift / left stick click |
| `IA_Crouch` | `bool` (held) | Ctrl / left bumper |
| `IA_Dodge` | `bool` (triggered) | Alt / right bumper |
| `IA_Mantle` | `bool` (triggered) | F / left bumper hold |
| `IA_Ability` | `bool` (triggered) | Q / left trigger |
| `IA_PrimaryFire` | `bool` | LMB / right trigger |
| `IA_SecondaryFire` | `bool` | RMB / left trigger |
| `IA_WeaponSwitch` | `float` (axis or triggered) | Scroll / dpad |

#### Input Mapping Contexts

- `IMC_OnFoot` — default ground/air movement
- `IMC_Ability` — layered on top, for ability-specific bindings
- `IMC_Vehicle` — reserved for future vehicle states

#### `FNPMoverInputCmd` (Mover Input Struct)

This struct is what Mover consumes each tick. It must implement `NetSerialize` for NPP.

```cpp
USTRUCT()
struct FNPMoverInputCmd
{
    GENERATED_BODY()

    // Normalised 2D movement intent in local space
    UPROPERTY() FVector2D MoveInput;

    // Normalised 2D look delta this frame
    UPROPERTY() FVector2D LookInput;

    // Bitfield for boolean actions (compact for NPP serialisation)
    // Bit 0: Jump pressed   Bit 1: Jump held
    // Bit 2: Sprint held    Bit 3: Crouch held
    // Bit 4: Dodge pressed  Bit 5: Mantle pressed
    UPROPERTY() uint8 ActionBits;

    // Desired orientation (camera yaw)
    UPROPERTY() FRotator ControlRotation;

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};
```

#### `ProduceInput` Flow

```
EnhancedInput callbacks accumulate state into a frame-local cache:
  FNPRawInputCache { MoveAxis, LookDelta, bJumpPressed, bSprintHeld, ... }

ANPCharacterPawn::ProduceInput(SimTimeMs, Context)
  → calls UNPMovementInputComponent::BuildInputCmd(cache)
  → writes FNPMoverInputCmd into Context
  → clears one-shot flags (jump, dodge) from cache
```

The separation between the raw cache (gathered during normal input callbacks, any frame rate) and `BuildInputCmd` (called at the Mover simulation rate) is critical for correctness under variable frame rates.

---

## 7. Phase 3 — Movement System (Mover Modes)

### Mover 2.0 Fundamentals

Mover processes movement in a pipeline:
1. `UMoverComponent::TickComponent` triggers simulation
2. `ProduceInput` is called to collect this tick's `FMoverInputCmdContext`
3. The active `UBaseMovementMode` has `SimulationTick` called
4. Any active `FLayeredMoveBase` entries also run and blend
5. The resulting `FMoverTickEndData` (position, velocity, orientation) is committed
6. NPP records the state for prediction/reconciliation

All movement modes are registered on `UMoverComponent` as sub-objects and referenced by name. Mode transitions are handled via `FMoverTransitionBase` subclasses registered alongside the modes.

### `FNPMoverSyncState`

Extended sync state that carries Netsper-specific per-tick data inside Mover's simulation.

```cpp
USTRUCT()
struct FNPMoverSyncState : public FMoverDefaultSyncState
{
    GENERATED_BODY()

    // Current stamina (mirrored here for movement mode queries during prediction)
    UPROPERTY() float CurrentSP;
    UPROPERTY() float MaxSP;

    // Active movement sub-state tags (crouching, wall-running side, etc.)
    UPROPERTY() FGameplayTag MovementSubState;

    // Time spent in current mode (for duration-limited modes)
    UPROPERTY() float ModeElapsedTime;

    // Stagger duration remaining (prevents full movement after hard landing)
    UPROPERTY() float StaggerTimeRemaining;

    bool NetSerialize(FArchive& Ar, UPackageMap*, bool&);
};
```

> **Design note:** SP is mirrored into the Mover sync state so movement modes can read/consume SP during prediction without needing to cross-simulation boundaries. The canonical SP value lives in `UNPStaminaComponent`; after each tick, the two are reconciled.

---

### 7.1 `UNPGroundMovementMode`

**Inherits:** `UBaseMovementMode`

Handles all grounded states: walking, sprinting, and crouching. Sprinting and crouching are sub-states within this mode, not separate modes, to avoid unnecessary mode transitions for common actions.

#### Sub-States (FGameplayTag stored in sync state)

| Tag | Description |
|-----|-------------|
| `Movement.Ground.Walk` | Default run state |
| `Movement.Ground.Sprint` | High speed, consumes SP/sec |
| `Movement.Ground.Crouch` | Reduced height, reduced speed |
| `Movement.Ground.CrouchSprint` | Not allowed — crouching cancels sprint |

#### `SimulationTick` Logic

```
1. Read FNPMoverInputCmd from context
2. Determine desired sub-state:
   a. bSprintHeld AND CurrentSP > 0 → Sprint sub-state
   b. bCrouchHeld → Crouch sub-state (also initiates slide if speed > SlideEntryThreshold)
   c. else → Walk sub-state
3. Resolve target speed from sub-state:
   WalkSpeed = 600 cm/s
   SprintSpeed = 950 cm/s
   CrouchSpeed = 320 cm/s
4. Apply horizontal acceleration with ground friction:
   Acceleration = 2800 cm/s² (walk), 3200 cm/s² (sprint)
   Deceleration = 1800 cm/s² (no input)
   Friction coefficient applied on top
5. Consume SP if sprinting:
   SprintSPCostPerSecond = 15 SP/s
   Write back to FNPMoverSyncState.CurrentSP
6. Handle capsule height change for crouch:
   Lerp capsule half-height to 30 cm (crouching)
7. Check transition conditions:
   a. Leave ground → request transition to AirMovementMode
   b. Slide entry conditions met → request transition to SlideMode
   c. Wallrun entry conditions met → request transition to WallRunMode
   d. Mantle entry conditions met → request transition to MantleMode
   e. StaggerTimeRemaining > 0 → reduce movement speed by stagger penalty
```

#### Cues Emitted

- `Cue.Movement.SprintStarted` / `Cue.Movement.SprintStopped`
- `Cue.Movement.CrouchStarted` / `Cue.Movement.CrouchStopped`
- `Cue.Stamina.SprintDepleted` (when SP hits 0 and sprint is cancelled)

---

### 7.2 `UNPAirMovementMode`

**Inherits:** `UBaseMovementMode`

Handles all airborne states including falling, air jumps, coyote time, and air dodge.

#### Configuration

```cpp
float CoyoteTimeDuration   = 0.15f;   // seconds after leaving ground, jump still allowed
float AirControlMultiplier = 0.35f;   // fraction of ground acceleration available in air
float GravityScale         = 1.8f;    // stronger than default for snappier feel
float TerminalVelocity     = -3000.f; // cm/s downward

// Jump parameters (Apex-inspired: variable height based on hold duration)
float JumpInitialVelocity  = 680.f;   // cm/s upward impulse
float JumpHoldGravityScale = 0.7f;    // reduced gravity while holding jump
float JumpHoldMaxDuration  = 0.25f;   // max seconds of jump-hold benefit
```

#### `SimulationTick` Logic

```
1. Apply gravity (scaled) to vertical velocity
2. Clamp to terminal velocity
3. Read air control input, apply horizontal acceleration * AirControlMultiplier
4. If bJumpHeld AND jumping AND JumpHoldTimer < JumpHoldMaxDuration:
   Apply reduced gravity (JumpHoldGravityScale)
5. Coyote time: track time since left ground; allow one jump within window
6. Landing detection: trace downward; if hit within landing threshold:
   Emit Cue.Movement.Landed(impactSpeed)
   if impactSpeed > StaggerThreshold AND no roll input → apply stagger
   if roll input detected → queue FNPLandingRollLayeredMove
   → request transition to GroundMovementMode
7. Wallrun entry check: left/right velocity component, near wall → WallRunMode
8. WallClimb entry check: forward velocity, facing wall → WallClimbMode
```

#### Air Dodge

Air dodge is a `FNPDodgeLayeredMove` queued when `bDodgePressed` is true in air. It is identical to ground dodge except the directional bias is purely horizontal and there is no ground friction.

---

### 7.3 `UNPSlideMode`

**Inherits:** `UBaseMovementMode`

A transitional mode entered from ground movement when sprint-crouching above the slide entry threshold.

#### Entry Conditions
- Active mode is `GroundMovementMode`
- `bCrouchHeld` is true
- `HorizontalSpeed >= SlideEntryThreshold` (700 cm/s)

#### `SimulationTick` Logic

```
1. On entry: record slide direction from current velocity
2. Apply momentum preservation: minimal friction during slide
3. Ramp detection via floor normal:
   if floor normal tilts forward (downhill): boost velocity proportionally
   if floor normal tilts backward (uphill): increase friction
4. SP-extended slide: if bSprintHeld AND CurrentSP > 0:
   Continue reduced friction even as speed drops below natural exit threshold
   SprintSPCostDuringSlide = 20 SP/s
5. Exit conditions (priority order):
   a. bCrouchHeld released AND speed < WalkSpeed → GroundMovementMode
   b. Left ground → AirMovementMode (carry slide momentum)
   c. Hit wall → check wallrun entry
6. Capsule remains crouched height during slide
```

#### Cues Emitted
- `Cue.Movement.SlideStarted`
- `Cue.Movement.SlideEnded`

---

### 7.4 `UNPWallRunMode`

**Inherits:** `UBaseMovementMode`

Allows traversal along vertical surfaces. Duration is physics-derived, not timer-based.

#### Entry Conditions

```
- In AirMovementMode
- Capsule side sweep hits a wall with:
  - Normal roughly horizontal (dot(normal, Up) < 0.3)
  - Horizontal speed > WallRunMinEntrySpeed (500 cm/s)
  - Angle of approach between 10°–80° relative to wall surface
- Player not already on the same wall (cooldown 0.5s per wall section)
```

#### Wall Side Detection

A left and right capsule side trace (offset 45 cm) determines which side the wall is on. This is stored as `Movement.WallRun.Left` or `Movement.WallRun.Right` in `MovementSubState`.

#### `SimulationTick` Logic

```
1. On entry: record wall normal, compute tangent run direction
   Adjust run direction to blend player's forward with wall tangent
2. Apply velocity along wall tangent (preserve entry speed)
3. Nullify gravity component (wall provides normal force)
4. Apply slight downward drift proportional to ModeElapsedTime:
   WallRunGravityDrift = 150 * (ModeElapsedTime / WallRunMaxDuration) cm/s²
   This creates a natural arc that limits duration without a hard cutoff
5. Camera tilt: write tilt target to blackboard for UNPCameraComponent
   LeftWall: +12° roll  RightWall: -12°  (smoothed on camera side)
6. Continuous wall trace to confirm wall still present:
   If trace fails → exit to AirMovementMode with exit velocity preserved
7. Exit conditions:
   a. Wall no longer traced → AirMovementMode
   b. bJumpPressed → FNPJumpLayeredMove (wall jump variant)
      Wall jump: push away from wall normal + upward
   c. Speed drops below WallRunMinContinueSpeed → AirMovementMode
   d. Reached top of wall → WallClimbMode (if bSprintHeld)
   e. ModeElapsedTime > WallRunMaxDuration → AirMovementMode
```

#### Cues Emitted
- `Cue.Movement.WallRunStarted(Side)`
- `Cue.Movement.WallRunEnded`
- `Cue.Movement.WallJumped`

---

### 7.5 `UNPWallClimbMode`

**Inherits:** `UBaseMovementMode`

Short vertical climbs inspired by Apex Legends' wall-climb. Distinct from wall-run: character faces the wall and moves upward.

#### Entry Conditions

```
- In AirMovementMode or GroundMovementMode
- Forward trace hits wall (normal roughly backward, -X relative to player)
- Upward velocity OR recent jump (within 0.3s)
- Wall height > threshold (wall must have climbable height remaining)
- Cooldown elapsed (0.8s since last climb on same surface)
```

#### `SimulationTick` Logic

```
1. On entry: lock horizontal input, drive character upward
   ClimbSpeed = 480 cm/s upward
2. Rotate character to face wall (yaw only, immediate snap)
3. Continuous forward trace to maintain wall contact
4. Exit conditions:
   a. Trace fails (top of wall reached) → AirMovementMode with upward boost
      "hop over" effect
   b. ModeElapsedTime > WallClimbMaxDuration (0.6s) → AirMovementMode
   c. bJumpPressed → mini wall-jump with forward/upward vector
```

---

### 7.6 `UNPMantleMode`

**Inherits:** `UBaseMovementMode`

Smooth ledge-grab-and-pull-up. Two variants: normal (animation-length) and SP-boosted (explosive hop).

#### Detection (runs as a check in `GroundMovementMode` and `AirMovementMode`)

```
Forward trace (capsule forward, max 80 cm) at chest height → hits wall
  If hit:
    Upward trace from impact point (+100 cm) → open space detected
    Ledge top height within mantle range (character top to 120 cm above it)
    → flag mantle candidate for next tick
```

#### `SimulationTick` Logic — Normal Mantle

```
1. On entry: record ledge top position
2. Phase 1 (hang): move character upward until eye level clears ledge
   VerticalSpeed = 350 cm/s
3. Phase 2 (pull-up): move forward over ledge
   HorizontalSpeed = 260 cm/s
4. Capsule remains crouched during mantle to avoid clipping
5. Exit: reached ledge top → GroundMovementMode, restore capsule height
6. Mantle duration cap: 0.8s total
```

#### SP-Boosted Mantle

```
Condition: bSprintHeld during mantle entry AND CurrentSP > SPBoostMantleCost (20 SP)
Behaviour: skip the animation phases entirely
  → apply upward impulse strong enough to clear the ledge as a jump
  → character appears to launch up over the ledge
  → transition to AirMovementMode briefly, then land on ledge
  → emit Cue.Movement.MantleBoostUsed
```

---

### 7.7 Layered Moves

Layered moves are short-duration movement overrides that blend with or replace the active mode's output.

#### `FNPDodgeLayeredMove`

**Purpose:** Quick directional dash (ground or air).

```
Duration: 0.2s
SP Cost: 25 SP (flat, on activation)
Input: dodge direction = normalise(MoveInput) or camera-forward if no input
VelocityBoost: 1200 cm/s in dodge direction
Friction: 0 during dodge
Exit: duration elapsed; blends out velocity over 0.05s
Cooldown: tracked in FNPMoverSyncState; dodge unavailable while SP < DodgeCost
```

Air dodge rules:
- Limited to 1 air dodge per airborne phase
- Resets on landing

#### `FNPJumpLayeredMove`

**Purpose:** Handles the jump impulse for all jump variants (ground, wall-jump, wall-run jump).

```
Variants (set at construction):
  Ground Jump:   +680 cm/s vertical
  Wall Jump:     +550 cm/s vertical + 650 cm/s away from wall normal
  WallRun Jump:  +600 cm/s vertical + 400 cm/s perpendicular to wall

Duration: 1 tick (impulse applied once)
Cues: Cue.Movement.Jumped(Variant)
```

#### `FNPLandingRollLayeredMove`

**Purpose:** Convert hard-landing downward velocity into forward momentum; prevent stagger.

```
Entry condition: Landed with |impactSpeed| > RollThreshold (800 cm/s)
  AND bCrouchHeld (roll input) within 0.15s window

On activation:
  Cancel stagger flag
  Apply forward momentum = |impactSpeed| * 0.6 cm/s in camera-forward direction
  Play roll animation via Cue.Movement.LandingRoll
  Duration: 0.35s (roll animation length)

If not rolled (hard landing above threshold):
  Apply stagger: FNPMoverSyncState.StaggerTimeRemaining = 0.4s
  Cue.Movement.LandingStagger(impactSpeed)
```

---

## 8. Phase 4 — Stamina Component (SP)

### `UNPStaminaComponent`

**Inherits:** `UActorComponent`

SP (Stamina Points) is the resource governing advanced movement and ability usage. The component maintains the canonical SP value and provides the `INPStaminaProvider` interface consumed by movement modes and the ability component.

#### Why not inside Mover's sync state directly?

Mover's sync state carries a *mirror* of SP for in-prediction queries, but the canonical value lives here to:
- Allow non-movement systems (abilities) to consume SP cleanly
- Support regen logic that runs outside the Mover simulation tick
- Keep Mover's sync state lean

#### `FNPStaminaSyncState`

Used for the NPP-predicted SP mirror only (written by Mover, read by modes):

```cpp
USTRUCT()
struct FNPStaminaSyncState
{
    GENERATED_BODY()
    UPROPERTY() float CurrentSP = 100.f;
    UPROPERTY() float MaxSP     = 100.f;
    UPROPERTY() float RegenRate = 18.f;   // SP/s
    UPROPERTY() float RegenDelay = 1.2f;  // seconds after last consumption
    UPROPERTY() float RegenDelayRemaining = 0.f;
};
```

#### `INPStaminaProvider` Interface

```cpp
class INPStaminaProvider
{
public:
    virtual float GetCurrentSP() const = 0;
    virtual float GetMaxSP() const = 0;

    // Returns true if sufficient SP available; deducts if so
    virtual bool TryConsumesSP(float Amount) = 0;

    // Forced consumption (for predicted deductions from movement modes)
    virtual void ConsumeSP(float Amount) = 0;

    // SP gain (pickups, ability effects)
    virtual void RestoreSP(float Amount) = 0;
};
```

#### Regen Logic

```
TickComponent (normal Actor tick, not Mover tick):
  if RegenDelayRemaining > 0:
    RegenDelayRemaining -= DeltaTime
  else if CurrentSP < MaxSP:
    delta = RegenRate * DeltaTime
    if currently sprinting: delta *= SprintRegenPenalty (0.5)
    if in combat (weapon recently fired): delta *= CombatRegenPenalty (0.3)
    CurrentSP = FMath::Min(CurrentSP + delta, MaxSP)
    Replicate via OnRep_CurrentSP

OnRep_CurrentSP: broadcast delegate for UI / audio
```

#### SP Costs Summary (from GDD)

| Action | Cost |
|--------|------|
| Sprint (per second) | 15 SP |
| Dodge | 25 SP |
| Extended slide (per second) | 20 SP |
| SP-boosted mantle | 20 SP (flat) |
| Ability-specific costs | Defined per ability |
| Ball carrier drain (Touchdown mode) | TBD by game mode |

---

## 9. Phase 5 — Custom Ability System (NPP)

### Design Goals

- Client predicts ability activation and cooldown locally
- Server is authoritative: enforces cooldowns, validates targets
- No GAS; all state in custom structs registered with NPP
- Abilities that affect movement inject `FLayeredMove` into `UMoverComponent`
- Abilities that affect the world (Shield, Wall) spawn actors server-side, replicated normally

### 9.1 NPP State Triads for Abilities

The ability component runs as a separate NPP simulation alongside Mover's simulation.

#### `FNPAbilityInputCmd`

```cpp
USTRUCT()
struct FNPAbilityInputCmd
{
    GENERATED_BODY()

    // Which ability to activate this tick (0 = none, 1 = primary ability slot)
    UPROPERTY() uint8 ActivationSlot;

    // Targeting data (where the camera is pointing for directional abilities)
    UPROPERTY() FVector_NetQuantize TargetLocation;
    UPROPERTY() FRotator TargetRotation;

    // Cancel request for channel/hold abilities
    UPROPERTY() bool bCancelActive;

    bool NetSerialize(FArchive& Ar, UPackageMap*, bool&);
};
```

#### `FNPAbilitySyncState`

```cpp
USTRUCT()
struct FNPAbilitySyncState
{
    GENERATED_BODY()

    // One slot (GDD: players pick 1 ability)
    // Cooldown remaining in simulation milliseconds
    UPROPERTY() int32 AbilityCooldownRemainingMs;

    // Duration remaining for channel/hold abilities (Flight, Invisibility)
    UPROPERTY() int32 AbilityDurationRemainingMs;

    // Which ability is loaded in the slot (FGameplayTag)
    UPROPERTY() FGameplayTag EquippedAbilityTag;

    // True while the ability is in active state (not just cooldown)
    UPROPERTY() bool bAbilityActive;

    // SP snapshot consumed on last activation (for reconciliation)
    UPROPERTY() float SPConsumedOnActivation;

    bool NetSerialize(FArchive& Ar, UPackageMap*, bool&);
};
```

#### `FNPAbilityAuxState`

Carries ability-specific scratch data that needs prediction (e.g., grapple target point, flight velocity contribution).

```cpp
USTRUCT()
struct FNPAbilityAuxState
{
    GENERATED_BODY()

    // Grapple: hook location (predicted)
    UPROPERTY() FVector_NetQuantize GrappleHookPoint;
    UPROPERTY() bool bGrappleHookAttached;

    // Flight: remaining fuel (predicted separately from SP for granularity)
    UPROPERTY() float FlightFuelRemaining;

    // Generic float for ability-specific use
    UPROPERTY() float AbilityStateFloat;

    bool NetSerialize(FArchive& Ar, UPackageMap*, bool&);
};
```

### 9.2 `UNPAbilityComponent`

**Inherits:** `UActorComponent` + NPP actor interface registration

The component manages the NPP ability simulation and owns the active `UNPAbilityBase` instance.

#### Key Members

```cpp
// The loaded ability instance (spawned as sub-object at equip time)
UPROPERTY() TObjectPtr<UNPAbilityBase> EquippedAbility;

// Current ability state (canonical, server-authoritative)
UPROPERTY(ReplicatedUsing=OnRep_AbilityState)
FNPAbilitySyncState ReplicatedState;

// NPP proxy handle
FNetworkPredictionProxy AbilityProxy;
```

#### Simulation Tick

```
SimulationTick(FNPAbilityInputCmd, FNPAbilitySyncState&, FNPAbilityAuxState&):

1. Tick cooldown: AbilityCooldownRemainingMs -= DeltaMs (clamp to 0)
2. Tick active duration: AbilityDurationRemainingMs -= DeltaMs (clamp to 0)
   if reached 0 AND bAbilityActive: trigger deactivation
3. If ActivationSlot > 0 AND bCancelActive:
   Deactivate active ability
4. If ActivationSlot > 0 AND NOT bAbilityActive AND CooldownRemainingMs == 0:
   Validate SP: TryConsumeSP(EquippedAbility->GetSPCost())
   If valid: activate ability
     bAbilityActive = true
     AbilityDurationRemainingMs = EquippedAbility->GetDuration()
     AbilityCooldownRemainingMs = 0 (set on deactivation)
     Call EquippedAbility->OnActivated(InputCmd, SyncState, AuxState, MoverComp)
5. If bAbilityActive:
   Call EquippedAbility->Tick(DeltaMs, SyncState, AuxState, MoverComp)
```

#### Ability-to-Mover Bridge

Abilities that need to influence movement do so by calling `UMoverComponent::QueueLayeredMove(...)`. This is safe to call from the ability simulation tick because:
- Both simulations run during the same frame
- The ability simulation is ordered before Mover's simulation via `TickPrerequisites`
- Layered moves are consumed by Mover in the same tick they are queued

### 9.3 `UNPAbilityBase`

Abstract base class for all ability implementations.

```cpp
UCLASS(Abstract, EditInlineNew)
class UNPAbilityBase : public UObject
{
public:
    // Intrinsic ability properties
    virtual FGameplayTag GetAbilityTag() const PURE_VIRTUAL(GetAbilityTag, return {};);
    virtual float GetSPCost() const { return 0.f; }
    virtual int32 GetCooldownMs() const PURE_VIRTUAL(GetCooldownMs, return 0;);
    virtual int32 GetDurationMs() const { return 0; }   // 0 = instant

    // Simulation callbacks (called from within NPP simulation tick)
    virtual void OnActivated(const FNPAbilityInputCmd&, FNPAbilitySyncState&,
                             FNPAbilityAuxState&, UMoverComponent*) {}
    virtual void OnTick(int32 DeltaMs, FNPAbilitySyncState&,
                        FNPAbilityAuxState&, UMoverComponent*) {}
    virtual void OnDeactivated(FNPAbilitySyncState&, FNPAbilityAuxState&) {}

    // Server-only: spawn/despawn world actors (shields, walls)
    virtual void OnActivatedAuthority(AActor* Owner, const FNPAbilityInputCmd&) {}
    virtual void OnDeactivatedAuthority(AActor* Owner) {}

    // Cue emission helper
    void EmitCue(FGameplayTag CueTag, const FNPAbilityInputCmd& Context);
};
```

### 9.4 Concrete Ability Implementations

#### `UNPGrappleAbility`

```
Tag:      Ability.Grapple
SP Cost:  30 SP
Cooldown: 8s
Duration: Until hook detaches or player cancels

OnActivated:
  Line trace from camera in look direction, MaxRange = 2000 cm
  If hit: record GrappleHookPoint in AuxState
          bGrappleHookAttached = true
          Queue FNPGrappleLayeredMove on MoverComponent
            The layered move computes pull force each tick toward GrappleHookPoint
            Damping at arrival
  If miss: instant fail, no cooldown triggered (or short 1s penalty)

OnTick:
  If bCancelActive OR rope length < DetachDistance:
    Detach (clear bGrappleHookAttached)
    Remove layered move

OnDeactivatedAuthority:
  Despawn rope visual actor

Inspired by Apex Legends: shorter range, good for repositioning,
does not halt horizontal momentum — adds to it.
```

#### `UNPShieldAbility`

```
Tag:      Ability.Shield
SP Cost:  0 (no SP cost, but has cooldown)
Cooldown: 12s (begins when shield breaks or expires)
Duration: 5s or until HP threshold depleted

OnActivatedAuthority:
  Spawn AShieldActor attached to player capsule
  AShieldActor blocks projectiles via custom collision channel
  Has shield HP pool (separate from player HP)

OnTick:
  Sync shield orientation to player camera (front-facing)

OnDeactivatedAuthority:
  Destroy AShieldActor
```

#### `UNPWallAbility`

```
Tag:      Ability.Wall
SP Cost:  0
Cooldown: 15s
Duration: 8s or until destroyed

OnActivatedAuthority:
  Line trace from player feet forward, find floor normal
  Spawn AWallActor at trace endpoint, aligned to floor
  AWallActor provides cover, blocks projectiles and movement
  Has HP pool; destroyed when depleted

OnDeactivated: AWallActor lifespan auto-expires or is destroyed
```

#### `UNPFlightAbility`

```
Tag:      Ability.Flight
SP Cost:  Continuous drain: 25 SP/s while active
Cooldown: 6s after fuel runs out (not after manual cancel)
Duration: Limited by SP

OnActivated:
  FlightFuelRemaining = CurrentSP (ability uses SP as fuel directly)

OnTick:
  Consume SP at FlightDrainRate
  If SP reaches 0: force deactivate
  Inject FNPFlightLayeredMove into MoverComponent:
    Overrides gravity
    Allows 6DoF directional flight based on camera orientation
    MaxFlightSpeed = 900 cm/s
    Acceleration = 1800 cm/s²

OnDeactivated:
  Remove FNPFlightLayeredMove
  Player enters AirMovementMode with current velocity retained
```

#### `UNPInvisibilityAbility`

```
Tag:      Ability.Invisibility
SP Cost:  0
Cooldown: 10s (begins when revealed or duration ends)
Duration: 5s or until broken

Invisibility break conditions:
  - Player fires a weapon
  - Player takes damage
  - Duration expires

OnActivated:
  SetActorHiddenInGame(true) on BodyMesh (server propagates to other clients)
  Reduce footstep sound radius to near-zero
  Apply subtle shimmer material overlay (visible at very close range)

OnDeactivated:
  SetActorHiddenInGame(false)
  Restore audio settings
  Start cooldown
```

---

## 10. Phase 6 — Health Component

### `UNPHealthComponent`

Standard replicated health (not NPP — health mutations come from server authority, no client-side prediction needed).

#### Key Members

```cpp
UPROPERTY(ReplicatedUsing=OnRep_Health)
float CurrentHealth;

UPROPERTY(EditDefaultsOnly)
float MaxHealth = 100.f;

FOnHealthChanged OnHealthChanged;   // broadcast delegate (C++ + Blueprint)
FOnDeath OnDeath;                   // broadcast delegate
```

#### Interface: `INPDamageable`

```cpp
class INPDamageable
{
public:
    virtual void ApplyDamage(float Amount, AActor* Instigator,
                              FGameplayTag DamageType) = 0;
    virtual void ApplyHeal(float Amount) = 0;
    virtual bool IsAlive() const = 0;
};
```

#### `ApplyDamage` Flow (Server Only)

```
1. Check IsAlive()
2. CurrentHealth -= Amount (clamped to 0)
3. Broadcast OnHealthChanged
4. If CurrentHealth <= 0:
   Broadcast OnDeath(Instigator)
   Call ANPCharacterPawn::OnDeath()
     → Disable input
     → Trigger death animation via Cue
     → Notify game mode for respawn
```

#### Replication

`OnRep_Health`: update HUD health bar, play low-health audio cue.

---

## 11. Phase 7 — Weapon Component

### `UNPWeaponComponent`

Manages up to 3 weapon slots (GDD: players carry 0–3 weapons). Handles equipping, switching, and delegating fire/reload to the active weapon.

#### Key Members

```cpp
// Max 3 slots per GDD
UPROPERTY(ReplicatedUsing=OnRep_Weapons)
TArray<TObjectPtr<UNPWeaponBase>> WeaponSlots;  // max 3

UPROPERTY(ReplicatedUsing=OnRep_ActiveWeaponIndex)
int32 ActiveWeaponIndex = 0;

TObjectPtr<UNPWeaponBase> GetActiveWeapon() const;
```

#### `UNPWeaponBase`

Abstract base for all weapons (ranged and melee). Concrete subclasses implement firing logic.

```cpp
UCLASS(Abstract)
class UNPWeaponBase : public UObject
{
public:
    virtual void StartFire(const FVector& Origin, const FRotator& Direction) {}
    virtual void StopFire() {}
    virtual void StartAltFire() {}
    virtual void Reload() {}

    virtual bool CanFire() const { return true; }
    virtual FGameplayTag GetWeaponTag() const PURE_VIRTUAL(GetWeaponTag, return {};);

    // Weapon stats — tuned per subclass
    float BaseDamage;
    float FireRate;       // rounds per second
    int32 AmmoPerClip;
    int32 CurrentAmmo;
    float ReloadTime;
};
```

#### Weapon Switching

```
OnWeaponSwitchInput(Direction):
  NewIndex = (ActiveWeaponIndex + Direction) % WeaponSlots.Num()
  if NewIndex != ActiveWeaponIndex:
    GetActiveWeapon()->OnUnequipped()
    ActiveWeaponIndex = NewIndex
    GetActiveWeapon()->OnEquipped()
    Server_SwitchWeapon(NewIndex)  [reliable RPC]
```

#### Network Strategy for Firing

Weapons use a **client-authoritative fire with server validation** approach:
- Client fires immediately (local hitscan/projectile) for responsiveness
- Sends `Server_Fire(Origin, Direction, Timestamp)` RPC
- Server validates timing, position plausibility, ammo count
- Server applies damage via `INPDamageable::ApplyDamage`
- Discrepancies (cheating, desync) corrected by server

---

## 12. Phase 8 — Camera Component

### `UNPCameraComponent`

**Inherits:** `UActorComponent`

Manages all camera effects without touching the camera transform directly. Effects are layered through `UCameraModifier` instances on the `APlayerCameraManager`.

> **Rule:** Never set `UCameraComponent` transform directly from gameplay code. All effects are additive modifiers.

#### Camera Modifiers Managed

| Modifier Class | Trigger | Effect |
|----------------|---------|--------|
| `UCameraModifier_SpeedFOV` | Speed changes | FOV scales from 90° (walk) to 100° (max sprint) |
| `UCameraModifier_LandingShake` | `Cue.Movement.LandingStagger` | Downward shake decaying over 0.3s |
| `UCameraModifier_WallRunTilt` | `Cue.Movement.WallRunStarted/Ended` | ±12° roll lerped over 0.15s |
| `UCameraModifier_DodgeKick` | `Cue.Movement.Dodged` | Tiny velocity-opposite kick over 0.1s |
| `UCameraModifier_SlideDown` | `Cue.Movement.SlideStarted` | Small negative pitch to emphasise low height |

#### SP-Speed FOV Logic

```
SpeedFraction = FMath::Clamp(HorizontalSpeed / MaxSprintSpeed, 0.f, 1.f)
TargetFOV = FMath::Lerp(BaseFOV, MaxFOV, SpeedFraction * SpeedFraction) // quadratic
CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, FOVInterpSpeed)
```

#### Wallrun Tilt

```
On WallRunStarted(Left):  TargetRoll = +12°
On WallRunStarted(Right): TargetRoll = -12°
On WallRunEnded:          TargetRoll = 0°
Each tick: CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, 8.f)
```

---

## 13. Cross-System Integration

### SP Flow Between Systems

```
UNPStaminaComponent (canonical SP)
    │
    ├── UNPGroundMovementMode: queries via INPStaminaProvider
    ├── UNPSlideMode: queries via INPStaminaProvider
    ├── FNPDodgeLayeredMove: deducts on activation
    ├── FNPJumpLayeredMove: no SP cost
    ├── UNPMantleMode: deducts on SP-boosted variant
    └── UNPAbilityComponent: deducts per ability cost/rate
```

The mirrored SP in `FNPMoverSyncState.CurrentSP` is written each tick from `UNPStaminaComponent`'s current value before the Mover simulation runs, ensuring movement mode predictions use the correct value. After reconciliation, any delta is applied back.

### Ability ↔ Movement Integration

```
Flight ability active:
  UNPFlightAbility::OnTick
    → MoverComponent->QueueLayeredMove(FNPFlightLayeredMove{...})
    The FNPFlightLayeredMove overrides gravity and maps look direction to velocity

Grapple ability active:
  UNPGrappleAbility::OnTick
    → MoverComponent->QueueLayeredMove(FNPGrappleLayeredMove{hook, pullStrength})
    The FNPGrappleLayeredMove adds pull force toward hook point each tick
    Compatible with all base movement modes (adds to their output)
```

### Cue System

Both Mover and NPP support **simulation cues** — events that need to happen exactly at the simulation time they occur (not delayed by network). Cues are the bridge to:
- Animation Blueprint: trigger transitions (slide enter/exit, wall-run blend)
- Audio: footsteps, ability sounds, impact sounds
- VFX: slide sparks, wallrun particle trail
- Camera: land shake, tilt

All custom cues inherit from `FMoverCue` (for movement events) or a base `FAbilityCue` struct (for ability events).

---

## 14. Networking Topology

### Authority Model

| System | Client Prediction | Server Authority | Reconciliation |
|--------|------------------|-----------------|----------------|
| Movement (position/velocity) | Yes (Mover NPP) | Yes | NPP auto-corrects |
| SP mirror in Mover sync state | Yes | Yes | NPP auto-corrects |
| SP canonical (StaminaComponent) | Optimistic local | Server sends OnRep | Snap to server |
| Ability cooldown | Yes (NPP ability sim) | Yes | NPP corrects |
| Ability active state | Yes (predicted) | Yes | Reconciled |
| Health | No (server-driven) | Yes | OnRep |
| Weapon firing | Client-local FX | Damage applied by server | RPC validation |
| World ability actors (Shield, Wall) | No | Server spawns, replicates | Normal actor replication |

### Lag Compensation

Weapon hitscan uses Epic's `ULagCompensationComponent` pattern (or a custom equivalent):
- Server maintains a rolling history of character capsule positions
- On `Server_Fire` RPC receipt, rewind relevant character to the fire timestamp
- Validate hit against rewound position
- Apply damage if valid

This is implemented in a `UNetsperLagCompensationComponent` (separate from the character plan, implemented alongside weapon system).

### Replication Settings

```cpp
// ANPCharacterPawn
bReplicates = true;
bAlwaysRelevant = false;     // use distance-based relevancy
NetUpdateFrequency = 60.f;
MinNetUpdateFrequency = 15.f;
NetCullDistanceSquared = 22500 * 22500; // 225m, tuned for arena size
```

---

## 15. Implementation Order

Implement in this sequence to always have a runnable state:

| Step | Deliverable | Dependencies |
|------|-------------|-------------|
| 1 | `.uproject` plugin config, `Build.cs` update | None |
| 2 | `ANPCharacterPawn` — capsule, mesh, camera, bare Mover | Step 1 |
| 3 | `UNPMovementInputComponent` — basic WASD + look | Step 2 |
| 4 | `UNPGroundMovementMode` — walk, sprint, crouch | Step 3 |
| 5 | `UNPAirMovementMode` + jump layered move | Step 4 |
| 6 | `UNPStaminaComponent` — SP resource, regen | Step 4 |
| 7 | SP integration into ground mode (sprint cost) | Step 6 |
| 8 | `UNPSlideMode` | Step 6 |
| 9 | `FNPDodgeLayeredMove` (ground + air) | Step 6 |
| 10 | `UNPWallRunMode` + wall jump | Step 5 |
| 11 | `UNPWallClimbMode` | Step 10 |
| 12 | `UNPMantleMode` (normal + SP-boosted) | Step 6 |
| 13 | `FNPLandingRollLayeredMove` + stagger | Step 5 |
| 14 | `UNPCameraComponent` — all camera modifiers | Step 4 |
| 15 | `UNPHealthComponent` | Step 2 |
| 16 | `UNPWeaponComponent` + `UNPWeaponBase` | Step 15 |
| 17 | `UNPAbilityComponent` — NPP simulation scaffolding | Step 6 |
| 18 | `UNPAbilityBase` + `UNPFlightAbility` (simplest) | Step 17 |
| 19 | `UNPGrappleAbility` (requires layered move) | Step 18 |
| 20 | `UNPShieldAbility`, `UNPWallAbility` (world actors) | Step 18 |
| 21 | `UNPInvisibilityAbility` | Step 18 |
| 22 | Network testing pass — verify NPP prediction correctness | All above |
| 23 | Animation Blueprint hookup (per cue events) | Step 22 |

---

*End of Character Implementation Plan*
