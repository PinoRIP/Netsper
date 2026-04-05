# Netsper — Unreal Engine 5.7 C++ Workspace Instructions

You are a senior Unreal Engine 5 C++ developer working on **Netsper** (working title: Novector), a high-velocity competitive arena FPS built on UE 5.7 with Mover 2.0, Network Prediction Plugin (NPP), and Enhanced Input.

Read project documentation in `Docs/` for design context. See `Docs/INDEX.md` for a guide to all documents.

---

## Core Principles

- **All gameplay code is C++.** Blueprints are used for asset wiring, animation graphs, UI, and tuning — never for gameplay logic.
- **Component architecture over inheritance.** Prefer adding/composing components on an Actor. Avoid deep class hierarchies.
- **Network-first thinking.** Every gameplay feature must work under client-server with NPP prediction. Consider authority, prediction, and reconciliation from the start.
- **Blueprint is the editor, C++ is the engine.** Expose `UPROPERTY` handles for designers to assign assets, tune values, and hook events — but logic stays in C++.

---

## Project Conventions

- **Naming prefix:** All project classes, structs, and interfaces use the `NP` prefix (e.g., `ANPCharacterPawn`, `UNPStaminaComponent`, `FNPMoverInputCmd`, `INPStaminaProvider`).
- **File names** match class names exactly (without the UE type prefix letter): `NPCharacterPawn.h`, `NPStaminaComponent.cpp`.
- **Source layout:** `Source/Netsper/` with subdirectories per domain: `Character/`, `Input/`, `Movement/`, `Stamina/`, `Abilities/`, `Health/`, `Weapons/`, `Camera/`.
- **Log category:** Define one per module/domain using `DECLARE_LOG_CATEGORY_EXTERN` / `DEFINE_LOG_CATEGORY`. Use `UE_LOG(LogNP, ...)` or domain-specific variants like `UE_LOG(LogNPMovement, ...)`.
- **GameplayTags** are NOT prefixed with NP — use feature-domain names directly (e.g., `Movement.State.Sliding`).

---

## C++ Standards

### Types and Containers
- Use UE types exclusively: `FString`, `FName`, `FText`, `TArray`, `TMap`, `TSet`, `TOptional`, `TVariant`.
- Never use STL containers (`std::vector`, `std::map`, `std::string`) unless interfacing with third-party C libraries.
- Use `int32`, `uint8`, `float`, `double` — never bare `int`, `unsigned`, `short`.
- Use `bool` (not `UBOOL` or integers as booleans).

### Memory and Object References
- `TObjectPtr<T>` for all `UPROPERTY` UObject pointer members.
- `TWeakObjectPtr<T>` when the reference does not imply ownership and the target may be destroyed.
- `TSubclassOf<T>` for class references exposed to Blueprint.
- `TSoftObjectPtr<T>` / `TSoftClassPtr<T>` for async-loadable asset references.
- Raw `T*` only for transient, non-UPROPERTY local/cached pointers (e.g., inside a function body). Never store raw pointers in UPROPERTY.
- Never use `new`/`delete` for UObjects. Use `NewObject<T>()`, `CreateDefaultSubobject<T>()`, or `SpawnActor<T>()`.

### Class Macros
- Every gameplay class must have `UCLASS()` and `GENERATED_BODY()`.
- Every reflected struct must have `USTRUCT(BlueprintType)` and `GENERATED_BODY()`.
- Every reflected enum must have `UENUM(BlueprintType)`.
- Interfaces: declare `UINTERFACE(MinimalAPI)` class with `GENERATED_BODY()`, then the `I`-prefixed native interface class with pure virtual methods.

### Headers and Includes
- Every header starts with `#pragma once`.
- Include the generated header first: `#include "MyClass.generated.h"` (must be the **last** include).
- Use `#include "CoreMinimal.h"` as the first include in most headers.
- Minimize includes in headers — use forward declarations aggressively.
- Use `#include` in `.cpp` files for everything the implementation needs.
- Never include a `.cpp` file.

### Forward Declarations
```cpp
// In headers, forward declare instead of including:
class UMoverComponent;
class UNPStaminaComponent;
struct FNPMoverInputCmd;
```

---

## UPROPERTY Rules

### Exposure Categories

| Scenario | Specifiers |
|----------|-----------|
| Designer-tunable constant | `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="...")` |
| Per-instance override | `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="...")` |
| Runtime read from BP | `UPROPERTY(BlueprintReadOnly, Category="...")` — no Edit specifier |
| Internal C++ only | `UPROPERTY()` — reflected for GC only, no BP access |
| Replicated | Add `Replicated` or `ReplicatedUsing=OnRep_X` |

### Asset References — CRITICAL RULE
**NEVER hardcode asset paths in C++.** Do not use `ConstructorHelpers::FObjectFinder`, `LoadObject`, `StaticLoadObject`, or string-based asset paths.

Instead, expose a `UPROPERTY` so the asset is selected in the editor:

```cpp
// CORRECT — designer picks the asset in a Blueprint or on the instance
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
TObjectPtr<USoundBase> JumpSound;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
TObjectPtr<UNiagaraSystem> DashVFX;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
TSubclassOf<UAnimInstance> AnimBPClass;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
TSubclassOf<UUserWidget> HUDWidgetClass;

// For large/streamable assets, use soft references:
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
TSoftObjectPtr<UStaticMesh> PickupMesh;
```

```cpp
// WRONG — hardcoded path, breaks on rename/move, forces load at construction
static ConstructorHelpers::FObjectFinder<USoundBase> JumpSoundFinder(TEXT("/Game/Audio/SFX/Jump"));
JumpSound = JumpSoundFinder.Object;
```

This applies to: meshes, materials, textures, sounds, Niagara systems, animation montages, data assets, data tables, widget classes, AI behavior trees, and any other asset type.

---

## UFUNCTION Rules

### Exposure Patterns

| Purpose | Specifiers |
|---------|-----------|
| BP can call from event graph | `UFUNCTION(BlueprintCallable, Category="...")` |
| BP can override (with C++ default) | `UFUNCTION(BlueprintNativeEvent, Category="...")` — implement `_Implementation` in C++ |
| BP must override (no C++ default) | `UFUNCTION(BlueprintImplementableEvent, Category="...")` — no C++ body |
| C++ only, reflected for RPCs | `UFUNCTION(Server, Reliable)` or `UFUNCTION(Client, Reliable)` |
| Multicast (cosmetic) | `UFUNCTION(NetMulticast, Unreliable)` — for VFX/SFX cues |
| Delegate binding | `UFUNCTION()` — minimum reflection for dynamic delegate binding |

### When to Use BlueprintImplementableEvent vs BlueprintNativeEvent

- **BlueprintImplementableEvent**: Use for cosmetic hooks that have NO required C++ logic — animation triggers, UI notifications, sound cues. The C++ side just declares the event; BP provides the implementation that references specific assets.
- **BlueprintNativeEvent**: Use when there is a sensible C++ default but designers may want to extend or replace behavior. Always implement `FunctionName_Implementation` in C++.

### Cosmetic BP Hooks Pattern
When gameplay logic triggers something cosmetic (VFX, SFX, camera shake), emit a BlueprintImplementableEvent or fire a delegate so BP handles the asset-specific response:

```cpp
// In header
UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Feedback")
void OnDodgePerformed(FVector DodgeDirection);

UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Feedback")
void OnDamageTaken(float Damage, const FHitResult& HitInfo);

// C++ gameplay code just calls:
OnDodgePerformed(DodgeDir);  // BP decides which VFX/SFX to play
```

---

## Blueprint Interop Strategy

### What Stays in C++
- All gameplay logic, simulation, state machines
- Movement modes and physics
- Ability activation, cooldowns, resource costs
- Damage calculations, health management
- Weapon fire logic, hit detection
- Network prediction and reconciliation
- Input processing and command generation
- Component initialization and lifecycle

### What Goes to Blueprint
- **Asset assignment**: Meshes, materials, sounds, VFX, animations — via exposed `UPROPERTY`
- **Animation Blueprints**: AnimBP graphs read C++ state via `BlueprintReadOnly` properties
- **UI/UMG widgets**: Widget Blueprints bound to C++ data
- **Cosmetic feedback**: Camera shakes, hit markers, UI flash — triggered by C++ events/delegates
- **Tuning & balancing**: Speed values, cooldowns, damage numbers — via `EditDefaultsOnly` properties
- **Level-specific logic**: Level Blueprints for triggers, sequences, map-specific behavior
- **Data-driven content**: Data Assets and Data Tables that C++ reads at runtime

### The Asset Boundary Rule
If something requires selecting a specific asset (mesh, sound, material, particle system, montage, widget class), that selection MUST happen in Blueprint/Editor, not in C++ code. C++ declares the typed property; Blueprint/Editor fills it.

### Delegates for Decoupling
Use delegates to let BP systems react to C++ events without C++ knowing about BP:

```cpp
// In header
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, Delta);

UPROPERTY(BlueprintAssignable, Category = "Health")
FOnHealthChanged OnHealthChanged;

// In cpp — fire the delegate, BP widgets/effects listen
OnHealthChanged.Broadcast(CurrentHealth, -DamageAmount);
```

---

## Error Handling

- Use `check()` / `checkf()` for invariants that must never be violated (equivalent to assert — crashes in development builds).
- Use `ensure()` / `ensureMsgf()` for conditions that should be true but can be recovered from — logs callstack once, continues execution.
- Use `verify()` for conditions where the expression has side effects and must always execute.
- Use `UE_LOG(LogNP, Warning, ...)` for recoverable runtime anomalies.
- Use `IsValid()` or `::IsValid(Ptr)` before dereferencing any UObject pointer that may be null or pending kill.
- Never silently swallow errors. If a function can fail, return a bool or enum result, or log clearly.

---

## Code Formatting

- Opening braces on the same line for functions, control flow, and class bodies (Allman or K&R — follow UE convention of opening brace on new line for function definitions).
- Tabs for indentation (UE convention).
- `PascalCase` for all types, functions, properties, and local variables.
- `bPascalCase` for boolean variables (prefix with lowercase `b`).
- `InPascalCase` or `OutPascalCase` for function parameters when clarity helps.
- `E` prefix for enums: `ENPMovementState`.
- One class per header/source pair.

---

## Documentation Standards

- Brief `/** */` doc comments on public UFUNCTION and UPROPERTY declarations.
- No redundant comments that restate the code.
- Comments explain *why*, not *what*.
- Use `// TODO(NP):` for known work items.
- Use `// HACK:` for temporary workarounds that must be revisited.

# Running the project

At the end of adding changes make sure the project compiles and runs. You can run the project by launching the Unreal Editor executable with the project file as an argument, like this:

& "D:\Epic\Unreal\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\ProjectRepos\Netsper\Netsper.uproject" -game -nullrhi -unattended -nosplash -nocrashdialog -stdout -FullStdOutLogOutput -forcelogflush -ExecCmds="Automation RunTests Project.Startup" -testexit="Automation Test Queue Empty"