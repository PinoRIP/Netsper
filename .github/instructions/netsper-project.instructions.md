---
description: "Use when working on Netsper project-specific features: character pawn, movement modes, stamina system, ability system, health, weapons, camera, or Enhanced Input integration. Covers NP naming conventions, component hierarchy, source layout, and project-specific architecture decisions."
---

# Netsper Project Conventions & Architecture

## Naming Convention — NP Prefix

ALL project classes use the `NP` prefix (not `Netsper`). This is a firm project-wide convention.

| Category | Pattern | Example |
|----------|---------|---------|
| Actor | `ANP<Name>` | `ANPCharacterPawn`, `ANPProjectile` |
| Component | `UNP<Name>Component` | `UNPStaminaComponent`, `UNPAbilityComponent` |
| UObject | `UNP<Name>` | `UNPAbilityBase`, `UNPWeaponBase` |
| Interface (UE) | `UNP<Name>` | `UNPStaminaProvider` (UINTERFACE class) |
| Interface (native) | `INP<Name>` | `INPStaminaProvider`, `INPDamageable` |
| Struct | `FNP<Name>` | `FNPMoverInputCmd`, `FNPMoverSyncState` |
| Enum | `ENP<Name>` | `ENPMovementState`, `ENPAbilityType` |
| Delegate | `FOn<Event>` | `FOnHealthChanged`, `FOnStaminaChanged` |
| Log category | `LogNP` or `LogNP<Domain>` | `LogNP`, `LogNPMovement`, `LogNPAbility` |
| GameplayTag | No prefix | `Movement.State.Sliding`, `Ability.Grapple` |

### File Naming

Files match class names exactly, minus the UE type prefix letter:

| Class | Header | Source |
|-------|--------|--------|
| `ANPCharacterPawn` | `NPCharacterPawn.h` | `NPCharacterPawn.cpp` |
| `UNPStaminaComponent` | `NPStaminaComponent.h` | `NPStaminaComponent.cpp` |
| `FNPMoverInputCmd` | `NPMoverTypes.h` | (types header, no .cpp) |
| `INPStaminaProvider` | `NPStaminaInterface.h` | (interface, no .cpp) |

---

## Source Directory Layout

```
Source/Netsper/
├── Netsper.Build.cs
├── Netsper.h / Netsper.cpp          # Module + LogNP category
│
├── Character/
│   └── NPCharacterPawn.h / .cpp     # Main pawn, component host
│
├── Input/
│   └── NPMovementInputComponent.h / .cpp  # Enhanced Input → Mover bridge
│
├── Movement/
│   ├── NPMoverTypes.h               # FNPMoverInputCmd, FNPMoverSyncState, ENPMovementState
│   ├── Modes/
│   │   ├── NPGroundMovementMode.h / .cpp
│   │   ├── NPAirMovementMode.h / .cpp
│   │   ├── NPSlideMode.h / .cpp
│   │   ├── NPWallRunMode.h / .cpp
│   │   ├── NPWallClimbMode.h / .cpp
│   │   └── NPMantleMode.h / .cpp
│   └── LayeredMoves/
│       ├── NPDodgeLayeredMove.h / .cpp
│       ├── NPJumpLayeredMove.h / .cpp
│       └── NPLandingRollLayeredMove.h / .cpp
│
├── Stamina/
│   ├── NPStaminaComponent.h / .cpp
│   ├── NPStaminaTypes.h             # FNPStaminaSyncState
│   └── NPStaminaInterface.h         # INPStaminaProvider
│
├── Abilities/
│   ├── NPAbilityComponent.h / .cpp
│   ├── NPAbilityTypes.h             # ENPAbilityType, FNPAbility structs
│   ├── NPAbilityBase.h / .cpp
│   └── Impl/
│       ├── NPGrappleAbility.h / .cpp
│       ├── NPShieldAbility.h / .cpp
│       ├── NPWallAbility.h / .cpp
│       ├── NPFlightAbility.h / .cpp
│       └── NPInvisibilityAbility.h / .cpp
│
├── Health/
│   ├── NPHealthComponent.h / .cpp
│   └── NPDamageInterface.h          # INPDamageable
│
├── Weapons/
│   ├── NPWeaponComponent.h / .cpp
│   └── NPWeaponBase.h / .cpp
│
└── Camera/
    ├── NPCameraComponent.h / .cpp
    └── NPCameraTypes.h
```

Place new files in the correct subdirectory. Create new subdirectories only for genuinely new domains.

---

## Component Hierarchy (ANPCharacterPawn)

```
ANPCharacterPawn (APawn, IMoverInputProducerInterface)
│
├── UCapsuleComponent (42r × 96h, root)
├── USkeletalMeshComponent (Body — 3P/shadow)
├── USkeletalMeshComponent (Arms — FP only, attached to camera)
├── USpringArmComponent (zero-length boom, eye height)
├── UCameraComponent (no lag)
│
├── UMoverComponent (NPP backend)
│   ├── UNPGroundMovementMode
│   ├── UNPAirMovementMode
│   ├── UNPSlideMode
│   ├── UNPWallRunMode
│   ├── UNPWallClimbMode
│   └── UNPMantleMode
│
├── UNPMovementInputComponent (Enhanced Input → Mover)
├── UNPStaminaComponent (SP resource, implements INPStaminaProvider)
├── UNPAbilityComponent (custom ability system, separate NPP sim)
├── UNPHealthComponent (HP + damage, implements INPDamageable)
├── UNPWeaponComponent (weapon slot management)
└── UNPCameraComponent (FOV, shake, tilt effects)
```

---

## Enhanced Input Integration

### Input Action Setup

```cpp
// Expose Input Action assets — assigned in Blueprint/Editor
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Move;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Look;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Jump;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Sprint;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Crouch;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Dodge;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Ability;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Fire;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Reload;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
TObjectPtr<UInputAction> IA_Interact;

// Input mapping contexts
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Contexts")
TObjectPtr<UInputMappingContext> IMC_OnFoot;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Contexts")
TObjectPtr<UInputMappingContext> IMC_Ability;
```

### Input Binding

```cpp
void UNPMovementInputComponent::SetupInputBindings(UEnhancedInputComponent* EIC)
{
	if (!IsValid(EIC)) return;

	EIC->BindAction(IA_Move,    ETriggerEvent::Triggered, this, &UNPMovementInputComponent::OnMoveTriggered);
	EIC->BindAction(IA_Move,    ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnMoveCompleted);
	EIC->BindAction(IA_Look,    ETriggerEvent::Triggered, this, &UNPMovementInputComponent::OnLookTriggered);
	EIC->BindAction(IA_Jump,    ETriggerEvent::Started,   this, &UNPMovementInputComponent::OnJumpStarted);
	EIC->BindAction(IA_Jump,    ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnJumpReleased);
	EIC->BindAction(IA_Sprint,  ETriggerEvent::Started,   this, &UNPMovementInputComponent::OnSprintStarted);
	EIC->BindAction(IA_Sprint,  ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnSprintReleased);
	EIC->BindAction(IA_Crouch,  ETriggerEvent::Started,   this, &UNPMovementInputComponent::OnCrouchStarted);
	EIC->BindAction(IA_Crouch,  ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnCrouchReleased);
	EIC->BindAction(IA_Dodge,   ETriggerEvent::Started,   this, &UNPMovementInputComponent::OnDodgeStarted);
}
```

### Accumulating Input for NPP

```cpp
void UNPMovementInputComponent::ProduceInput(int32 SimTimeMs, FMoverTickStartData& StartData)
{
	FNPMoverInputCmd& Cmd = StartData.GetMutableInputCmd<FNPMoverInputCmd>();

	Cmd.MoveInput = CachedMoveInput;
	Cmd.ViewRotation = CachedViewRotation;
	Cmd.bWantsJump = bJumpPressed;
	Cmd.bWantsSprint = bSprintHeld;
	Cmd.bWantsCrouch = bCrouchHeld;
	Cmd.bWantsDodge = bDodgeConsumed;

	// Consume single-fire inputs
	bDodgeConsumed = false;
}
```

---

## Key Gameplay Constants

These are the design-specified values. Always use `EditDefaultsOnly` properties with these as defaults:

### Movement Speeds (cm/s)
| Parameter | Value |
|-----------|-------|
| Walk Speed | 600 |
| Sprint Speed | 950 |
| Crouch Speed | 300 |
| Slide Initial Speed | 700+ |
| Wall Run Speed | Tangent-derived |
| Wall Climb Speed | 480 |
| Mantle Speed | 260–350 |
| Dodge Speed | 1200 |

### Physics
| Parameter | Value |
|-----------|-------|
| Capsule Radius | 42 cm |
| Capsule Half-Height | 96 cm |
| Crouch Half-Height | 60 cm |
| Gravity Scale | 1.8x |
| Terminal Velocity | -3000 cm/s |
| Coyote Time | 0.15s |
| Jump Impulse (Ground) | 680 cm/s |

### Stamina
| Parameter | Value |
|-----------|-------|
| Max SP | 100 |
| Sprint Cost | 15 SP/s |
| Dodge Cost | 25 SP |
| Extended Slide Cost | 20 SP/s |
| Mantle Boost Cost | 20 SP |
| Base Regen | 18 SP/s |
| Regen Delay | 1.2s |
| Sprint Regen Penalty | 0.5x |
| Combat Regen Penalty | 0.3x |

### Abilities
| Ability | SP Cost | Cooldown | Duration |
|---------|---------|----------|----------|
| Grapple | 30 SP | 8s | Instant |
| Shield | 0 SP | 12s | 5s |
| Wall | 0 SP | 15s | 8s |
| Flight | 25 SP/s | 6s | SP-limited |
| Invisibility | 0 SP | 10s | 5s |

---

## Implementation Phase Order

When implementing features, follow the dependency order from `Docs/IMPLEMENTATION_CHECKLIST.md`:

1. Project Configuration (plugins, modules)
2. Core Pawn (`ANPCharacterPawn`)
3. Input System (`UNPMovementInputComponent`)
4. Ground Movement (`UNPGroundMovementMode`)
5. Air Movement (`UNPAirMovementMode`)
6. Stamina System (`UNPStaminaComponent`)
7. Sprint & SP Integration
8. Slide Mode (`UNPSlideMode`)
9. Wall Run (`UNPWallRunMode`)
10. Wall Climb (`UNPWallClimbMode`)
11. Mantle (`UNPMantleMode`)
12. Dodge & Landing Roll (Layered Moves)
13. Camera System (`UNPCameraComponent`)
14. Health System (`UNPHealthComponent`)
15. Weapon System (`UNPWeaponComponent`, `UNPWeaponBase`)
16–21. Ability System + 5 Abilities
22. Network Testing
23. Animation Integration
