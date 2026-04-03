# NP Naming Convention Guide

## Quick Reference

All Netsper classes use the **`NP`** prefix instead of the full project name.

### Why NP?

- **Scannability:** 2 characters vs 7 = 40% shorter
- **Consistency:** Industry standard for game projects (UE, Unreal uses 1-2 char prefixes)
- **Agent-friendly:** Easier to parse and generate in code

---

## Naming Convention Rules

### Classes

```cpp
// Movement
UNPCharacterPawn              // Core player character
UNPMovementInputComponent     // Input → Mover command translation
UNPGroundMovementMode         // Walk, sprint, crouch
UNPAirMovementMode            // Jump, falling, air control
UNPSlideMode                  // Momentum slide
UNPWallRunMode                // Vertical wall traversal
UNPWallClimbMode              // Short wall climb
UNPMantleMode                 // Ledge vault

// Resources
UNPStaminaComponent           // SP resource management
UNPHealthComponent            // HP and damage

// Abilities
UNPAbilityComponent           // Ability system (NPP simulation)
UNPAbilityBase                // Abstract base for all abilities
UNPGrappleAbility             // Grapple hook ability
UNPShieldAbility              // Defensive shield
UNPWallAbility                // Temporary wall cover
UNPFlightAbility              // Short-burst flight
UNPInvisibilityAbility        // Stealth ability

// Combat
UNPWeaponComponent            // Weapon slot manager
UNPWeaponBase                 // Abstract base for weapons

// Camera
UNPCameraComponent            // Camera effects manager

// Network (special cases)
UNPLagCompensationComponent   // Lag compensation for hitscan
```

### Interfaces

```cpp
INPStaminaProvider            // SP query interface for movement modes
INPDamageable                 // Damage/heal interface
```

### Structs

```cpp
// Movement input/state
FNPMoverInputCmd              // Movement input command
FNPMoverSyncState             // Movement sync state (extended)

// Stamina
FNPStaminaSyncState           // SP state mirror for Mover

// Layered moves
FNPDodgeLayeredMove           // Dodge dash effect
FNPJumpLayeredMove            // Jump impulse effect
FNPLandingRollLayeredMove     // Landing roll effect

// Abilities
FNPAbilityInputCmd            // Ability activation input
FNPAbilitySyncState           // Ability state (cooldown, duration, etc.)
FNPAbilityAuxState            // Ability-specific auxiliary data
```

### Enums

```cpp
// Example (if needed):
enum class ENPMovementMode
{
    Ground,
    Air,
    Slide,
    WallRun,
    WallClimb,
    Mantle
};
```

### Files

Follow the same naming as the class:

```
Source/Netsper/
├── Character/
│   ├── NPCharacterPawn.h
│   └── NPCharacterPawn.cpp
├── Movement/
│   ├── NPMoverTypes.h
│   ├── Modes/
│   │   ├── NPGroundMovementMode.h
│   │   ├── NPGroundMovementMode.cpp
│   │   └── ...
│   └── LayeredMoves/
│       ├── NPDodgeLayeredMove.h
│       ├── NPDodgeLayeredMove.cpp
│       └── ...
├── Stamina/
│   ├── NPStaminaComponent.h
│   ├── NPStaminaComponent.cpp
│   ├── NPStaminaTypes.h
│   └── NPStaminaInterface.h
├── Abilities/
│   ├── NPAbilityComponent.h
│   ├── NPAbilityComponent.cpp
│   ├── NPAbilityTypes.h
│   ├── NPAbilityBase.h
│   ├── NPAbilityBase.cpp
│   └── Impl/
│       ├── NPGrappleAbility.h
│       ├── NPGrappleAbility.cpp
│       └── ...
├── Health/
│   ├── NPHealthComponent.h
│   ├── NPHealthComponent.cpp
│   └── NPDamageInterface.h
├── Weapons/
│   ├── NPWeaponComponent.h
│   ├── NPWeaponComponent.cpp
│   ├── NPWeaponBase.h
│   └── NPWeaponBase.cpp
└── Camera/
    ├── NPCameraComponent.h
    ├── NPCameraComponent.cpp
    └── NPCameraTypes.h
```

---

## Using the NP Prefix in Code

### In .h files (includes)

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NPMoverTypes.h"
#include "NPStaminaInterface.h"

#include "NPCharacterPawn.generated.h"

class UNPMovementInputComponent;
class UNPStaminaComponent;
class UNPAbilityComponent;

UCLASS()
class NETSPER_API ANPCharacterPawn : public APawn, public IMoverInputProducerInterface
{
    GENERATED_BODY()
    
public:
    ANPCharacterPawn();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
    TObjectPtr<UNPMovementInputComponent> MovementInputComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gameplay")
    TObjectPtr<UNPStaminaComponent> StaminaComponent;
    
    // ...
};
```

### In .cpp files

```cpp
#include "Character/NPCharacterPawn.h"
#include "Input/NPMovementInputComponent.h"
#include "Stamina/NPStaminaComponent.h"
#include "Abilities/NPAbilityComponent.h"

ANPCharacterPawn::ANPCharacterPawn()
{
    // Create components
    MovementInputComponent = CreateDefaultSubobject<UNPMovementInputComponent>(TEXT("MovementInput"));
    StaminaComponent = CreateDefaultSubobject<UNPStaminaComponent>(TEXT("Stamina"));
    AbilityComponent = CreateDefaultSubobject<UNPAbilityComponent>(TEXT("Ability"));
    
    // ...
}
```

---

## Common Patterns

### Casting

```cpp
// Check if interface
if (INPStaminaProvider* StaminaProvider = Cast<INPStaminaProvider>(Other))
{
    float CurrentSP = StaminaProvider->GetCurrentSP();
}

// Casting components
if (UNPStaminaComponent* StaminaComp = GetOwner()->FindComponentByClass<UNPStaminaComponent>())
{
    StaminaComp->ConsumeSP(25.0f);
}
```

### Delegates

```cpp
DECLARE_MULTICAST_DELEGATE_TwoParams(FNPStaminaChanged, float, float); // Current, Max

UPROPERTY(BlueprintAssignable, Category="Stamina")
FNPStaminaChanged OnStaminaChanged;
```

### GameplayTags

Tags don't use the NP prefix (they use the feature name):

```cpp
// ✅ Correct
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("Movement.Ground.Sprint"));
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("Ability.Grapple"));

// ❌ Wrong
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("NP.Movement.Ground.Sprint"));
```

---

## Summary Checklist

- [ ] All new classes use `NP` prefix (U/A/I prefix + NP + feature name)
- [ ] File names match class names (NPClassName.h / NPClassName.cpp)
- [ ] Struct names use `FNP` prefix
- [ ] Interface names use `INP` prefix
- [ ] GameplayTags use feature names (no NP prefix)
- [ ] Includes reference correct NP paths
- [ ] Forward declarations use full NP names
- [ ] Comments refer to classes by their NP names

---

*Last Updated: April 3, 2026*

