# Netsper Implementation — Pre-Development Checklist

**Document Date:** April 3, 2026  
**Status:** Ready for Agent Handoff  
**Target:** 23-Phase Character System Implementation

---

## ✅ Documentation Complete

- [x] Character Implementation Plan (1392 lines) — All features specified
- [x] Game Design Document (822 lines) — All requirements documented
- [x] NP Naming Convention Guide — Agent-ready
- [x] Implementation Plan Review — Validation complete

---

## Phase-by-Phase Implementation Checklist

### Phase 1: Project Configuration ✅
- [ ] Enable plugins in `Netsper.uproject`:
  - [ ] Mover
  - [ ] NetworkPrediction
  - [ ] EnhancedInput
  - [ ] GameplayTags
  - [ ] ModelingToolsEditorMode (editor only)
- [ ] Update `Netsper.Build.cs` with module dependencies:
  - [ ] Core, CoreUObject, Engine, InputCore
  - [ ] EnhancedInput
  - [ ] Mover, MoverExamples
  - [ ] NetworkPrediction, NetCore
  - [ ] GameplayTags
  - [ ] PhysicsCore (private)
- [ ] Update `DefaultEngine.ini`:
  - [ ] Network bandwidth settings
  - [ ] NPP MaxSimulationFrames = 128
  - [ ] Mover bUseNetworkPredictionBackend = true

---

### Phase 2: Core Pawn ✅
- [ ] Create `ANPCharacterPawn` class
- [ ] Implement component hierarchy:
  - [ ] UCapsuleComponent (root, 96cm tall, 42cm radius)
  - [ ] USkeletalMeshComponent (body mesh, hidden in FP)
  - [ ] USkeletalMeshComponent (arms mesh, FP-only)
  - [ ] USpringArmComponent (zero-length boom, eye-height)
  - [ ] UCameraComponent (no lag)
  - [ ] UMoverComponent
- [ ] Implement `IMoverInputProducerInterface::ProduceInput()`
- [ ] Create getter functions for all components
- [ ] Set replication properties:
  - [ ] bReplicates = true
  - [ ] NetUpdateFrequency = 60
  - [ ] MinNetUpdateFrequency = 15
  - [ ] NetCullDistanceSquared = 225m²

---

### Phase 3: Input System ✅
- [ ] Create `UNPMovementInputComponent`
- [ ] Define `FNPMoverInputCmd` struct with NetSerialize
- [ ] Implement Enhanced Input mapping contexts:
  - [ ] IMC_OnFoot (default)
  - [ ] IMC_Ability (layered)
  - [ ] IMC_Vehicle (reserved)
- [ ] Define input actions (IA_Move, IA_Look, IA_Jump, etc.)
- [ ] Implement `ProduceInput()` delegation
- [ ] Test input accumulation and one-shot flag clearing

---

### Phase 4: Ground Movement ✅
- [ ] Create `UNPGroundMovementMode`
- [ ] Define `FNPMoverSyncState` struct with NetSerialize
- [ ] Implement sub-state logic (Walk, Sprint, Crouch)
- [ ] Set movement speeds:
  - [ ] Walk: 600 cm/s
  - [ ] Sprint: 950 cm/s
  - [ ] Crouch: 320 cm/s
- [ ] Implement acceleration/deceleration
- [ ] Implement capsule height transition (96cm → 60cm crouch)
- [ ] Test ground-to-air transition logic

---

### Phase 5: Air Movement ✅
- [ ] Create `UNPAirMovementMode`
- [ ] Implement jump logic (680 cm/s vertical)
- [ ] Implement apex-style variable jump height
- [ ] Set gravity scale: 1.8x
- [ ] Implement coyote time: 0.15s window
- [ ] Create `FNPJumpLayeredMove` for impulse
- [ ] Implement landing detection
- [ ] Test wallrun/wallclimb entry conditions

---

### Phase 6: Stamina System ✅
- [ ] Create `UNPStaminaComponent`
- [ ] Define `FNPStaminaSyncState` struct
- [ ] Create `INPStaminaProvider` interface
- [ ] Implement SP regen:
  - [ ] Base: 18 SP/s
  - [ ] Delay: 1.2s after last consumption
  - [ ] Sprint penalty: 0.5x
  - [ ] Combat penalty: 0.3x
- [ ] Create replication via OnRep_CurrentSP
- [ ] Test SP consumption patterns

---

### Phase 7: Sprint & SP Integration ✅
- [ ] Update `UNPGroundMovementMode` to consume SP
- [ ] Sprint cost: 15 SP/s
- [ ] Test sprint starvation when SP < cost
- [ ] Emit cues: SprintStarted, SprintStopped, SprintDepleted
- [ ] Test SP regen balance

---

### Phase 8: Slide Mode ✅
- [ ] Create `UNPSlideMode`
- [ ] Entry condition: speed ≥ 700 cm/s, crouch held
- [ ] Implement momentum preservation
- [ ] Implement ramp detection (speed boost on downhill)
- [ ] SP extended slide: 20 SP/s cost
- [ ] Exit conditions: speed drops, ground lost, etc.
- [ ] Emit cues: SlideStarted, SlideEnded

---

### Phase 9: Dodge Ability ✅
- [ ] Create `FNPDodgeLayeredMove`
- [ ] Duration: 0.2s
- [ ] Cost: 25 SP (flat, on activation)
- [ ] Velocity: 1200 cm/s in input direction
- [ ] Ground dodge: available always
- [ ] Air dodge: 1 per airborne phase, resets on landing
- [ ] Blend out over 0.05s at end
- [ ] Test dodge state tracking

---

### Phase 10: Wallrun Mode ✅
- [ ] Create `UNPWallRunMode`
- [ ] Entry: side sweep hit wall, speed ≥ 500 cm/s
- [ ] Detect wall side (left/right) for camera tilt
- [ ] Preserve entry speed along wall tangent
- [ ] Apply gravity drift over time (duration limit)
- [ ] Exit: wall trace fails, speed drops, or duration exceeded
- [ ] Wall jump variant: 600 cm/s up + 400 cm/s perpendicular
- [ ] Emit cues: WallRunStarted, WallRunEnded, WallJumped

---

### Phase 11: Wall Climb Mode ✅
- [ ] Create `UNPWallClimbMode`
- [ ] Entry: forward trace hits wall, upward velocity or recent jump
- [ ] Climb speed: 480 cm/s upward
- [ ] Lock horizontal input, face wall
- [ ] Exit: trace fails (top reached), duration exceeded, or jump input
- [ ] Climb jump: forward/upward vector
- [ ] Cooldown: 0.8s per surface

---

### Phase 12: Mantle Mode ✅
- [ ] Create `UNPMantleMode`
- [ ] Detection: forward trace at chest height + upward trace
- [ ] Normal mantle: 2 phases (hang up, pull over)
  - [ ] Vertical speed: 350 cm/s
  - [ ] Horizontal speed: 260 cm/s
  - [ ] Duration cap: 0.8s
- [ ] SP-boosted variant: instant impulse up (if SP ≥ 20)
- [ ] Emit cues: MantleStarted, MantleEnded, MantleBoostUsed
- [ ] Test mantle height ranges

---

### Phase 13: Landing Roll ✅
- [ ] Create `FNPLandingRollLayeredMove`
- [ ] Landing stagger: apply if impact speed > 800 cm/s
- [ ] Roll input window: 0.15s after landing
- [ ] Roll effect: convert downward velocity to forward momentum (0.6x)
- [ ] Duration: 0.35s (animation length)
- [ ] Emit cues: LandingStagger, LandingRoll
- [ ] Test stagger penalty on movement speed

---

### Phase 14: Camera System ✅
- [ ] Create `UNPCameraComponent`
- [ ] Implement camera modifiers (no direct transform edits):
  - [ ] SpeedFOV: 90°-100° based on sprint speed (quadratic lerp)
  - [ ] LandingShake: downward shake on hard landing
  - [ ] WallRunTilt: ±12° roll when on walls
  - [ ] DodgeKick: small opposite-velocity kick
  - [ ] SlideDown: slight negative pitch
- [ ] Test modifier layering and interpolation

---

### Phase 15: Health System ✅
- [ ] Create `UNPHealthComponent`
- [ ] Create `INPDamageable` interface
- [ ] Implement damage flow:
  - [ ] Server validates and applies damage
  - [ ] Broadcast OnHealthChanged
  - [ ] Trigger OnDeath when HP ≤ 0
- [ ] Create death flow:
  - [ ] Disable input
  - [ ] Emit death cue
  - [ ] Notify game mode for respawn
- [ ] Test replication via OnRep_Health

---

### Phase 16: Weapon System ✅
- [ ] Create `UNPWeaponComponent`
- [ ] Create `UNPWeaponBase` abstract class
- [ ] Implement weapon slots (max 3)
- [ ] Implement weapon switching:
  - [ ] Cycle active index
  - [ ] Call OnEquipped/OnUnequipped
  - [ ] Server RPC validation
- [ ] Implement firing network model:
  - [ ] Client fires immediately (local prediction)
  - [ ] Server_Fire RPC with timestamp
  - [ ] Server validates and applies damage
  - [ ] Implement lag compensation (rewind logic)
- [ ] Test weapon state replication

---

### Phase 17: Ability Component (NPP Scaffold) ✅
- [ ] Create `UNPAbilityComponent`
- [ ] Define `FNPAbilityInputCmd` struct with NetSerialize
- [ ] Define `FNPAbilitySyncState` struct with NetSerialize
- [ ] Define `FNPAbilityAuxState` struct with NetSerialize
- [ ] Implement NPP integration:
  - [ ] Register simulation tick
  - [ ] Handle input production
  - [ ] Manage state reconciliation
- [ ] Implement ability lifecycle:
  - [ ] Cooldown tick-down
  - [ ] Duration tick-down
  - [ ] Activation validation (SP check)
  - [ ] Callbacks: OnActivated, OnTick, OnDeactivated
- [ ] Test NPP prediction synchronization

---

### Phase 18: Ability Base & Flight ✅
- [ ] Create `UNPAbilityBase` abstract class
- [ ] Create `UNPFlightAbility`
- [ ] Flight specs:
  - [ ] Tag: Ability.Flight
  - [ ] Cost: 25 SP/s (continuous drain)
  - [ ] Cooldown: 6s after fuel depletion
  - [ ] Duration: limited by SP
  - [ ] Speed: 900 cm/s max
  - [ ] Acceleration: 1800 cm/s²
- [ ] Create `FNPFlightLayeredMove`
- [ ] Test flight entry/exit and SP drain

---

### Phase 19: Grapple Ability ✅
- [ ] Create `UNPGrappleAbility`
- [ ] Specs:
  - [ ] Tag: Ability.Grapple
  - [ ] Cost: 30 SP
  - [ ] Cooldown: 8s
  - [ ] Range: 2000 cm
  - [ ] Damping at arrival
- [ ] Create `FNPGrappleLayeredMove`
- [ ] Implement hook detection
- [ ] Implement rope visual actor
- [ ] Test grapple pull dynamics

---

### Phase 20: Shield & Wall Abilities ✅
- [ ] Create `UNPShieldAbility`
  - [ ] Tag: Ability.Shield
  - [ ] Cost: 0 SP
  - [ ] Cooldown: 12s
  - [ ] Duration: 5s
  - [ ] Create AShieldActor
- [ ] Create `UNPWallAbility`
  - [ ] Tag: Ability.Wall
  - [ ] Cost: 0 SP
  - [ ] Cooldown: 15s
  - [ ] Duration: 8s
  - [ ] Create AWallActor
- [ ] Implement server-side actor spawning
- [ ] Test collision and damage blocking

---

### Phase 21: Invisibility Ability ✅
- [ ] Create `UNPInvisibilityAbility`
- [ ] Specs:
  - [ ] Tag: Ability.Invisibility
  - [ ] Cost: 0 SP
  - [ ] Cooldown: 10s
  - [ ] Duration: 5s
  - [ ] Break on: fire weapon, take damage, duration end
- [ ] Implement visibility toggle
- [ ] Reduce footstep audio radius
- [ ] Apply shimmer material overlay
- [ ] Test break conditions

---

### Phase 22: Network Testing ✅
- [ ] Test Mover NPP prediction:
  - [ ] Client predicts movement correctly
  - [ ] Server sends corrections
  - [ ] Client reconciles without artifacts
- [ ] Test ability NPP prediction:
  - [ ] Cooldown prediction/reconciliation
  - [ ] Ability state synchronization
  - [ ] Cross-system (ability → movement) effects
- [ ] Test bandwidth usage:
  - [ ] Monitor net traffic at 60Hz
  - [ ] Verify within 32 kbps budget
- [ ] Test lag scenarios:
  - [ ] 50ms, 100ms, 150ms latency
  - [ ] Packet loss 5%
  - [ ] Verify prediction correctness at all latencies
- [ ] Test desync recovery

---

### Phase 23: Animation Blueprint Integration ✅
- [ ] Create animation blueprint for character
- [ ] Hook cues to animation states:
  - [ ] Ground states: Walk, Sprint, Crouch
  - [ ] Air states: Fall, Jump apex hold
  - [ ] Slide: enter animation → loop → exit
  - [ ] Wallrun: side-specific blends
  - [ ] Landing: stagger vs roll
  - [ ] Mantle: hang → pull phases
  - [ ] Ability activations per cue
- [ ] Test animation blending
- [ ] Test animation sync with movement ticks

---

## Pre-Implementation Validation

- [ ] All phase dependencies verified
- [ ] All class names use NP prefix
- [ ] All files follow NP naming convention
- [ ] Module dependencies reviewed
- [ ] Plugin versions confirmed compatible
- [ ] Team has read: CharacterImplementationPlan.md
- [ ] Team has read: NP_NAMING_CONVENTION.md
- [ ] Build.cs and .uproject ready for modification

---

## Risk Mitigation

| Risk | Mitigation |
|------|-----------|
| NPP prediction desync | Extensive network testing (Phase 22) required |
| Ability-movement interaction complexity | Layered move abstraction isolates concerns |
| SP economy balance | Tuning constants provided; playtest frequently |
| Animation sync with fast movement | High netupdate frequency (60Hz) mitigates |
| Wallrun/climb edge cases | Physics-based entry/exit (not time-based) |

---

## Success Criteria

- [ ] All 23 phases implemented and integrated
- [ ] Character moves at required speeds (600–950 cm/s)
- [ ] NPP prediction passes phase 22 network testing
- [ ] All abilities activate and interact correctly
- [ ] Animation blueprint responds to all cues
- [ ] Health/damage system functional
- [ ] Weapon system fires and validates on server
- [ ] SP regen balances with consumption
- [ ] No NPP reconciliation artifacts visible to player

---

## Quick Links

- **Implementation Plan:** `Docs/CharacterImplementationPlan.md`
- **Game Design Document:** `Docs/GDD.md`
- **Naming Convention:** `Docs/NP_NAMING_CONVENTION.md`
- **Review Summary:** `Docs/REVIEW_SUMMARY.md`

---

*Prepared: April 3, 2026 — Ready for Agent Implementation*

