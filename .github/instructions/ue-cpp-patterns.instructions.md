---
description: "Use when writing or editing Unreal Engine C++ header or source files. Covers UE5 class macros, UPROPERTY/UFUNCTION specifiers, memory management, type usage, include order, forward declarations, and code style."
applyTo: "Source/**/*.h, Source/**/*.cpp"
---

# Unreal Engine 5 C++ Patterns

## Header File Structure

Every `.h` file must follow this exact include order:

```cpp
#pragma once

#include "CoreMinimal.h"
#include <UE framework headers>
#include <project headers>
#include "ThisClass.generated.h"  // ALWAYS last include
```

The `.generated.h` include **must** be the last `#include` directive. UHT (Unreal Header Tool) will fail to parse the file otherwise.

## Forward Declarations Over Includes

In headers, forward-declare instead of including whenever a full type definition is not needed:

```cpp
// Forward declarations — no include needed
class UMoverComponent;
class UNPStaminaComponent;
class UInputAction;
class UInputMappingContext;
struct FNPMoverInputCmd;

// Only include what is actually needed for inheritance, inline code, or sizeof
#include "GameFramework/Pawn.h"  // Needed because we inherit from APawn
```

**When you must include:** base class header, types used in inline functions, types whose size is needed (members by value, not by pointer).

## UCLASS Patterns

### Actor-Derived Classes
```cpp
UCLASS()
class NETSPER_API ANPCharacterPawn : public APawn
{
	GENERATED_BODY()

public:
	ANPCharacterPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComp;
};
```

### Component Classes
```cpp
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPStaminaComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
```

### Abstract Base Classes
```cpp
UCLASS(Abstract)
class NETSPER_API UNPAbilityBase : public UObject
{
	GENERATED_BODY()
	// ...
};
```

## USTRUCT Patterns

### Replicated Data Structs
```cpp
USTRUCT(BlueprintType)
struct FNPMoverSyncState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	ENPMovementState MovementState = ENPMovementState::Ground;
};
```

### Input Command Structs (NPP)
```cpp
USTRUCT()
struct FNPMoverInputCmd
{
	GENERATED_BODY()

	FVector2D MoveInput = FVector2D::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	bool bWantsJump = false;
	bool bWantsSprint = false;
	bool bWantsCrouch = false;

	/** Custom NetSerialize for bandwidth optimization. */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FNPMoverInputCmd> : public TStructOpsTypeTraitsBase2<FNPMoverInputCmd>
{
	enum
	{
		WithNetSerializer = true,
	};
};
```

## UENUM Patterns

```cpp
UENUM(BlueprintType)
enum class ENPMovementState : uint8
{
	Ground    UMETA(DisplayName = "Ground"),
	Air       UMETA(DisplayName = "Air"),
	Slide     UMETA(DisplayName = "Slide"),
	WallRun   UMETA(DisplayName = "Wall Run"),
	WallClimb UMETA(DisplayName = "Wall Climb"),
	Mantle    UMETA(DisplayName = "Mantle"),
};
```

## Interface Pattern

```cpp
// In NPStaminaInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPStaminaInterface.generated.h"

UINTERFACE(MinimalAPI)
class UNPStaminaProvider : public UInterface
{
	GENERATED_BODY()
};

class NETSPER_API INPStaminaProvider
{
	GENERATED_BODY()

public:
	virtual bool TryConsumeStamina(float Amount) = 0;
	virtual float GetCurrentStamina() const = 0;
	virtual float GetMaxStamina() const = 0;
};
```

## UPROPERTY Specifier Quick Reference

```cpp
// Designer-tunable constant (set once in Blueprint class defaults, read at runtime)
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
float MaxSprintSpeed = 950.0f;

// Per-instance override (each placed actor can have a different value)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
float DamageMultiplier = 1.0f;

// Runtime state readable by BP (health bar widgets, HUD)
UPROPERTY(BlueprintReadOnly, Category = "Health")
float CurrentHealth;

// Replicated with OnRep callback
UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Health")
float CurrentHealth;

// Internal C++ only — reflected for GC, not exposed to BP
UPROPERTY()
TObjectPtr<UNPAbilityBase> ActiveAbility;

// Clamped value with metadata
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0", ClampMax = "200.0"))
float MaxStamina = 100.0f;

// Component reference — private with meta access for BP
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
TObjectPtr<UNPStaminaComponent> StaminaComponent;
```

## UFUNCTION Specifier Quick Reference

```cpp
// Blueprint callable from event graph
UFUNCTION(BlueprintCallable, Category = "Movement")
void StartSprinting();

// Blueprint pure function (no execution pin)
UFUNCTION(BlueprintPure, Category = "Stamina")
float GetStaminaPercent() const;

// Blueprint can override, C++ provides default
UFUNCTION(BlueprintNativeEvent, Category = "Damage")
float ModifyDamage(float BaseDamage, const FDamageEvent& DamageEvent);
// Must implement: float ANPCharacterPawn::ModifyDamage_Implementation(...)

// Blueprint must implement (no C++ body)
UFUNCTION(BlueprintImplementableEvent, Category = "Feedback")
void OnAbilityActivated(ENPAbilityType AbilityType);

// Server RPC — client requests, server executes
UFUNCTION(Server, Reliable, WithValidation)
void ServerRequestAbility(ENPAbilityType AbilityType);
// Must implement: void ServerRequestAbility_Implementation(...)
// Must implement: bool ServerRequestAbility_Validate(...)

// Client RPC — server sends to owning client
UFUNCTION(Client, Reliable)
void ClientAbilityDenied(ENPAbilityType AbilityType, FString Reason);

// Multicast — server sends to all clients (cosmetic only)
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayHitEffect(FVector HitLocation, FVector HitNormal);
```

## Memory Management Rules

```cpp
// Creating components in constructor
MyComponent = CreateDefaultSubobject<UNPStaminaComponent>(TEXT("StaminaComponent"));

// Spawning actors at runtime
FActorSpawnParameters SpawnParams;
SpawnParams.Owner = this;
ANPProjectile* Projectile = GetWorld()->SpawnActor<ANPProjectile>(ProjectileClass, SpawnTransform, SpawnParams);

// Creating UObjects at runtime
UNPAbilityBase* NewAbility = NewObject<UNPAbilityBase>(this, AbilityClass);

// NEVER do this:
// UNPAbilityBase* Bad = new UNPAbilityBase();  // No raw new for UObjects
// delete Bad;                                    // No raw delete for UObjects
```

## Pointer Safety

```cpp
// Always validate before use
if (IsValid(StaminaComponent))
{
	StaminaComponent->TryConsumeStamina(Cost);
}

// For interface checks
if (AActor* Owner = GetOwner())
{
	if (INPStaminaProvider* StaminaProvider = Cast<INPStaminaProvider>(Owner))
	{
		StaminaProvider->TryConsumeStamina(Cost);
	}
}

// TWeakObjectPtr for non-owning references
TWeakObjectPtr<AActor> TargetActor;
if (TargetActor.IsValid())
{
	// Safe to use
}
```

## Casting Patterns

```cpp
// Checked cast — returns nullptr if wrong type (most common, use in gameplay)
UNPStaminaComponent* Stamina = Cast<UNPStaminaComponent>(Component);

// Unchecked cast — asserts in debug, undefined in shipping (use when type is guaranteed)
UNPStaminaComponent* Stamina = CastChecked<UNPStaminaComponent>(Component);

// Interface cast
INPStaminaProvider* Provider = Cast<INPStaminaProvider>(Actor);

// NEVER use static_cast or dynamic_cast on UObjects — always use Cast<T>()
```

## Log Category Definition

```cpp
// In header (e.g., Netsper.h or domain header)
DECLARE_LOG_CATEGORY_EXTERN(LogNP, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogNPMovement, Log, All);

// In source (e.g., Netsper.cpp or domain source)
DEFINE_LOG_CATEGORY(LogNP);
DEFINE_LOG_CATEGORY(LogNPMovement);

// Usage
UE_LOG(LogNP, Warning, TEXT("Stamina depleted for %s"), *GetNameSafe(GetOwner()));
UE_LOG(LogNPMovement, Verbose, TEXT("Mode transition: %s -> %s"), *OldModeName, *NewModeName);
```

## GetLifetimeReplicatedProps

Every class with replicated properties must implement:

```cpp
void ANPCharacterPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPCharacterPawn, CurrentHealth);
	DOREPLIFETIME_CONDITION(ANPCharacterPawn, bIsSprinting, COND_SkipOwner);
}
```

## Constructor Best Practices

```cpp
ANPCharacterPawn::ANPCharacterPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set tick requirements
	PrimaryActorTick.bCanEverTick = true;

	// Create components
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.0f, 96.0f);
	SetRootComponent(CapsuleComp);

	// Network replication
	bReplicates = true;
	SetReplicatingMovement(false); // NPP handles movement replication

	// DO NOT load assets in constructor — expose UPROPERTY to BP instead
}
```

## Timers and Delegates

```cpp
// Timer usage (prefer over manual delta accumulation)
FTimerHandle RegenTimerHandle;
GetWorld()->GetTimerManager().SetTimer(
	RegenTimerHandle,
	this,
	&UNPStaminaComponent::RegenTick,
	0.1f,  // Rate
	true   // Looping
);

// Dynamic multicast delegates (BP-bindable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStamina);

UPROPERTY(BlueprintAssignable, Category = "Stamina")
FOnStaminaChanged OnStaminaChanged;

// Native multicast delegates (C++ only, better performance)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnModeChanged, ENPMovementState);
FOnModeChanged OnModeChanged;
```

## Gameplay Tags

```cpp
// Request tags — NOT prefixed with NP
FGameplayTag Tag_Movement_State_Sliding = FGameplayTag::RequestGameplayTag(FName("Movement.State.Sliding"));

// Tag containers
FGameplayTagContainer BlockedTags;
BlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Block.Movement")));

// Tag queries
if (ActiveTags.HasTag(Tag_Movement_State_Sliding))
{
	// Currently sliding
}
```
