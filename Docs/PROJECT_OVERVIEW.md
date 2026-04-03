# Netsper Project Overview — April 3, 2026

## Status: ✅ READY FOR IMPLEMENTATION

The Netsper character system implementation plan has been **thoroughly reviewed, validated, and standardized**. The project is now ready for agent-driven development.

---

## What Is Netsper?

**Netsper** (working title: **Novector**) is a high-velocity competitive arena FPS built on Unreal Engine 5.7 with:

- **Movement mastery** as the core skill pillar
- **6+ advanced movement modes** (wallrun, wallclimb, mantle, slide, dodge, etc.)
- **Stamina-based resource system** (SP) governing mobility
- **5 unique eSper abilities** with networked prediction
- **True first-person camera** with reactive effects
- **Component-based architecture** using Mover 2.0 and Network Prediction Plugin

---

## Project Scope

### Core Systems (Priority 1 — In Scope)

| System | Status | Complexity |
|--------|--------|------------|
| Movement (6 modes + layered moves) | Documented | ⭐⭐⭐⭐⭐ |
| Stamina Resource (SP) | Documented | ⭐⭐⭐⭐ |
| Custom Ability System (NPP) | Documented | ⭐⭐⭐⭐⭐ |
| Health & Damage | Documented | ⭐⭐ |
| Weapons (0-3 slots) | Documented | ⭐⭐⭐ |
| Camera System | Documented | ⭐⭐⭐ |
| Input Handling | Documented | ⭐⭐⭐ |
| Network Prediction | Documented | ⭐⭐⭐⭐⭐ |

### Secondary Systems (Priority 2 — Out of Scope)

- Character cosmetics and skins
- Progression and leveling
- Seasonal content

---

## Architecture at a Glance

```
ANPCharacterPawn (APawn)
├── Movement
│   ├── UMoverComponent (Mover 2.0)
│   │   ├── 6 Movement Modes
│   │   └── 3 Layered Moves
│   └── UNPMovementInputComponent
│
├── Resources
│   └── UNPStaminaComponent (SP management)
│
├── Abilities
│   └── UNPAbilityComponent (NPP-powered)
│       └── 5 Abilities (Grapple, Shield, Wall, Flight, Invisibility)
│
├── Combat
│   ├── UNPHealthComponent
│   └── UNPWeaponComponent (0-3 weapons)
│
└── Effects
    └── UNPCameraComponent (modifiers: FOV, shake, tilt)
```

**Key Principle:** Every system is a **self-contained, replicable component** that communicates via interfaces and cues.

---

## Documentation Structure

### 📋 Primary Documents

1. **CharacterImplementationPlan.md** (1392 lines)
   - Complete technical specification
   - 23 phases with pseudocode
   - All class/struct definitions
   - Networking topology

2. **GDD.md** (822 lines)
   - Design vision and pillars
   - Gameplay mechanics (high-level)
   - Game modes (Touchdown, Chaser, 1v1 Arena, BetterRoyal)
   - Progression and cosmetics

### 📖 Supporting Documents (NEW)

3. **NP_NAMING_CONVENTION.md**
   - Naming rules for all classes/structs
   - File structure conventions
   - Code examples and patterns

4. **IMPLEMENTATION_CHECKLIST.md**
   - Phase-by-phase task list
   - Success criteria per phase
   - Risk mitigation strategies

5. **REVIEW_SUMMARY.md**
   - Validation results (100% GDD coverage)
   - Changes applied (NP prefix)
   - Agent readiness assessment

---

## Key Numbers

| Metric | Value |
|--------|-------|
| **Total Implementation Phases** | 23 |
| **Movement Modes** | 6 (Ground, Air, Slide, WallRun, WallClimb, Mantle) |
| **Layered Moves** | 3 (Dodge, Jump, LandingRoll) |
| **Abilities** | 5 (Grapple, Shield, Wall, Flight, Invisibility) |
| **Core Components** | 8 |
| **Primary Classes** | 36+ |
| **Struct Definitions** | 8+ |
| **Interface Contracts** | 2 |
| **NPP Simulations** | 2 (Movement + Abilities) |
| **Estimated Dev Time** | 6-8 weeks (per agent capacity) |

---

## Movement Mechanics Quick Reference

| Mechanic | Speed | Cost | Duration | Notes |
|----------|-------|------|----------|-------|
| Walk | 600 cm/s | — | — | Base movement |
| Sprint | 950 cm/s | 15 SP/s | Stamina-limited | Increases FOV |
| Crouch | 320 cm/s | — | — | Reduces height |
| Jump | ↑680 cm/s | — | 1 tick | Apex-style (variable) |
| Coyote Jump | ↑680 cm/s | — | 0.15s window | Jump after leaving ground |
| Dodge | ±1200 cm/s | 25 SP | 0.2s | Ground + air (1 air per phase) |
| Slide | 700+ cm/s | 20 SP/s (extended) | Momentum-based | Ramp-sensitive |
| Wallrun | Tangent | — | Physics-derived | Gravity drift limits duration |
| WallClimb | ↑480 cm/s | — | 0.6s max | Short vertical climb |
| Mantle | 260-350 cm/s | 20 SP (boosted) | 0.8s normal | Two phases (hang, pull) |
| Landing Roll | ↑ | — | 0.35s | Converts downward velocity |

---

## Stamina Economy

| Action | Cost | Notes |
|--------|------|-------|
| Sprint | 15 SP/s | Regen penalty while sprinting (0.5x) |
| Dodge | 25 SP (flat) | Available if SP ≥ cost |
| Slide (extended) | 20 SP/s | Beyond natural momentum |
| Mantle (SP-boosted) | 20 SP (flat) | Instant impulse variant |
| Flight | 25 SP/s (drain) | Ability uses SP as fuel |
| Grapple | 30 SP (flat) | Ability activation cost |
| **Regen** | +18 SP/s | After 1.2s of no consumption |
| Regen (Sprint) | +9 SP/s | 0.5x penalty |
| Regen (Combat) | +5.4 SP/s | 0.3x penalty |

---

## Network Architecture

### Movement Prediction
- **Client:** Predicts locally via Mover NPP backend
- **Server:** Authoritative simulation + validation
- **Reconciliation:** NPP auto-corrects client desync

### Ability Prediction
- **Client:** Predicts cooldown + activation locally
- **Server:** Enforces authority, validates SP cost
- **Reconciliation:** NPP corrects state drift

### Weapon Firing
- **Client:** Fires immediately (hitscan/projectile)
- **Server:** Validates via lag compensation + applies damage
- **Authority Model:** Client-predictive with server validation

---

## Implementation Phases — Quick Summary

| Phase | Deliverable | Complexity |
|-------|-------------|-----------|
| 1-2 | Project setup + Core pawn | ⭐⭐ |
| 3 | Input system | ⭐⭐⭐ |
| 4-7 | Ground movement + stamina | ⭐⭐⭐⭐ |
| 8-13 | Advanced modes + layered moves | ⭐⭐⭐⭐⭐ |
| 14 | Camera system | ⭐⭐⭐ |
| 15-16 | Health + weapons | ⭐⭐ |
| 17-21 | Ability system + 5 abilities | ⭐⭐⭐⭐⭐ |
| 22 | Network testing | ⭐⭐⭐⭐ |
| 23 | Animation integration | ⭐⭐⭐ |

---

## Why This Architecture Works

### Separation of Concerns
- Mover handles movement prediction
- NPP handles ability prediction
- Layered moves bridge systems without coupling
- Components own their state

### Network Efficiency
- All predicted state in structs (optimal for NPP)
- Client prediction for responsiveness
- Server authority for fairness
- Bandwidth budget: 32 kbps with ~60 Hz update

### Scalability
- Component-based design = modular additions
- Interface contracts = easy swapping
- Cue system = decoupled animation/audio
- Multi-character support ready

### Testing
- Each component independently testable
- Phases can be developed in parallel (after dependencies)
- Network testing isolated to phase 22
- Animation integration last (no blocking)

---

## Getting Started

### For Developers

1. Read `CharacterImplementationPlan.md` (sections 1-4)
2. Review `NP_NAMING_CONVENTION.md` for coding standards
3. Check `IMPLEMENTATION_CHECKLIST.md` for your assigned phases
4. Implement phases sequentially, testing as you go

### For Leads

1. Review `REVIEW_SUMMARY.md` for validation results
2. Distribute phases among team members (23 phases)
3. Use `IMPLEMENTATION_CHECKLIST.md` for progress tracking
4. Schedule network testing for phase 22

### For QA

1. Review `GDD.md` for gameplay requirements
2. Use `IMPLEMENTATION_CHECKLIST.md` success criteria
3. Prepare test cases for each movement mechanic
4. Network stress test at phase 22

---

## Notes on This Implementation Plan

✅ **What's Good:**
- Complete technical specification (no ambiguity)
- Correct architecture for networked prediction
- Realistic physics-based movement logic
- Clear dependency graph (23 phases)

⚠️ **Limitations:**
- Animation integration deferred to phase 23 (animation team needed)
- Lag compensation architecture specified but not fully detailed
- Game mode specifics (Touchdown, Chaser, etc.) not part of character system
- Cosmetics/progression out of scope

📝 **To be Done Before Implementation:**
- [ ] Create blueprint base classes for pawn/components
- [ ] Set up Enhanced Input mapping contexts
- [ ] Create animation skeleton and locomotion anims
- [ ] Design ability VFX/SFX cues
- [ ] Prepare server infrastructure for playtesting

---

## Success Criteria — Final Validation

The implementation is considered **complete** when:

1. ✅ All 23 phases pass code review
2. ✅ Network testing (phase 22) confirms:
   - No prediction artifacts at 50/100/150ms latency
   - SP and cooldown reconciliation correct
   - Bandwidth within 32 kbps budget
3. ✅ Animation blueprint responds to all cues
4. ✅ Player can perform all documented mechanics fluently
5. ✅ Abilities interact correctly with movement
6. ✅ Health/weapon system functional
7. ✅ No obvious desync or lag compensation issues

---

## Questions? Reference These Docs

| Question | Document |
|----------|----------|
| "What should I name this class?" | `NP_NAMING_CONVENTION.md` |
| "What do I implement in phase X?" | `IMPLEMENTATION_CHECKLIST.md` |
| "Why this architecture?" | `REVIEW_SUMMARY.md` |
| "What's the detailed spec?" | `CharacterImplementationPlan.md` |
| "What are the design goals?" | `GDD.md` |

---

**Status:** Ready for implementation  
**Last Updated:** April 3, 2026  
**Next Step:** Distribute phases to development team

🚀 **Let's build this!**

