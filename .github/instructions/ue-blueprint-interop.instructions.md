---
description: "Use when implementing features that need Blueprint interaction, asset references, cosmetic feedback, UI binding, animation integration, or any C++-to-Blueprint boundary. Covers the asset boundary rule, delegate patterns, BlueprintImplementableEvent vs BlueprintNativeEvent, and what must NOT be in C++."
applyTo: "Source/**/*.h, Source/**/*.cpp"
---

# Blueprint Interop — The C++/Blueprint Boundary

## The Cardinal Rule

**C++ is the engine. Blueprint is the editor.**

- ALL gameplay logic, simulation, state, networking, and prediction lives in C++.
- ALL asset selection, cosmetic feedback, UI wiring, animation graphs, and tuning lives in Blueprint/Editor.
- The boundary between them is **typed UPROPERTY handles** and **delegate/event hooks**.

---

## NEVER Hardcode Asset Paths in C++

This is the single most important Blueprint interop rule. Violating it creates brittle code that breaks on asset rename/move and defeats the purpose of the editor.

### Forbidden Patterns
```cpp
// WRONG — ALL of these are forbidden
static ConstructorHelpers::FObjectFinder<USoundBase> SFX(TEXT("/Game/Audio/Jump"));
static ConstructorHelpers::FClassFinder<UUserWidget> HUD(TEXT("/Game/UI/BP_HUD"));
UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/Pickup"));
auto* DA = Cast<UDataAsset>(StaticLoadObject(UDataAsset::StaticClass(), nullptr, TEXT("/Game/Data/WeaponData")));
FSoftObjectPath Path(TEXT("/Game/FX/HitSpark"));
```

### Correct Patterns — Expose to Editor

```cpp
// Direct reference — loaded with the owning object
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
TObjectPtr<USoundBase> JumpSound;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
TObjectPtr<UNiagaraSystem> DashVFX;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
TObjectPtr<UMaterialInterface> HitDecalMaterial;

// Class reference — lets BP pick which subclass to spawn
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
TSubclassOf<ANPProjectile> ProjectileClass;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
TSubclassOf<UUserWidget> HUDWidgetClass;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
TSubclassOf<UAnimInstance> AnimBlueprintClass;

// Soft reference — async load on demand, doesn't bloat memory
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
TSoftObjectPtr<UStaticMesh> PickupMesh;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
TSoftObjectPtr<UNiagaraSystem> ExplosionEffect;

// Soft class reference — async load a Blueprint class
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
TSoftClassPtr<ANPCharacterPawn> BotPawnClass;
```

### Using Soft References at Runtime
```cpp
void ANPPickup::SpawnMesh()
{
	if (PickupMesh.IsNull())
	{
		return;
	}

	// Synchronous resolve (only if already loaded or small asset)
	if (UStaticMesh* Mesh = PickupMesh.Get())
	{
		MeshComponent->SetStaticMesh(Mesh);
		return;
	}

	// Async load
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(PickupMesh.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ANPPickup::OnMeshLoaded));
}

void ANPPickup::OnMeshLoaded()
{
	if (UStaticMesh* Mesh = PickupMesh.Get())
	{
		MeshComponent->SetStaticMesh(Mesh);
	}
}
```

---

## Asset Types and Where They Belong

| Asset Type | C++ Declares | Blueprint/Editor Fills |
|-----------|-------------|----------------------|
| Mesh (Static/Skeletal) | `TObjectPtr<UStaticMesh>` | Pick mesh in details panel |
| Material / Material Instance | `TObjectPtr<UMaterialInterface>` | Assign material in details |
| Sound (Cue/Wave/Base) | `TObjectPtr<USoundBase>` | Pick sound in details |
| Niagara System | `TObjectPtr<UNiagaraSystem>` | Pick VFX in details |
| Animation Montage | `TObjectPtr<UAnimMontage>` | Pick montage in details |
| Animation Blueprint | `TSubclassOf<UAnimInstance>` | Pick AnimBP class |
| Widget Blueprint | `TSubclassOf<UUserWidget>` | Pick widget class |
| Data Asset | `TObjectPtr<UMyDataAsset>` | Create DA in editor, assign |
| Data Table | `TObjectPtr<UDataTable>` | Create DT in editor, assign |
| Curve (Float/Vector) | `TObjectPtr<UCurveFloat>` | Create curve in editor |
| Camera Shake | `TSubclassOf<UCameraShakeBase>` | Pick shake class |
| Behavior Tree | `TObjectPtr<UBehaviorTree>` | Pick BT in editor |
| Texture / Render Target | `TObjectPtr<UTexture2D>` | Pick texture in details |
| Physical Material | `TObjectPtr<UPhysicalMaterial>` | Pick phys mat in details |

---

## Cosmetic Feedback — Events and Delegates

When C++ gameplay logic triggers something cosmetic (VFX, SFX, camera effects, screen shake, UI flash), C++ should NOT handle the asset-specific response. Instead:

### Pattern 1: BlueprintImplementableEvent (simplest)

Use when the owning Blueprint class handles the cosmetic directly.

```cpp
// Header — C++ declares the event, provides NO implementation
UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Feedback")
void OnDodgePerformed(FVector DodgeDirection);

UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Feedback")
void OnSlideStarted();

UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Feedback")
void OnWallRunStarted(bool bIsLeftWall);

UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Feedback")
void OnDamageTaken(float Damage, const FHitResult& HitInfo);

UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Feedback")
void OnKillConfirmed(AActor* VictimActor);
```

```cpp
// Source — C++ just fires the event at the right time
void UNPDodgeLayeredMove::OnDodgeExecuted(const FVector& Direction)
{
	if (ANPCharacterPawn* Pawn = Cast<ANPCharacterPawn>(GetOwner()))
	{
		Pawn->OnDodgePerformed(Direction);  // BP handles VFX/SFX
	}
}
```

### Pattern 2: Dynamic Multicast Delegate (decoupled, multiple listeners)

Use when multiple systems (UI widget, audio component, VFX manager) need to react.

```cpp
// Header
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityStateChanged, ENPAbilityType, AbilityType, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

// In the component class
UPROPERTY(BlueprintAssignable, Category = "Health|Events")
FOnHealthChanged OnHealthChanged;

UPROPERTY(BlueprintAssignable, Category = "Health|Events")
FOnPlayerDied OnPlayerDied;
```

```cpp
// Source — broadcast when state changes
void UNPHealthComponent::ApplyDamage(float Damage)
{
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);  // UI updates, hit effects play

	if (CurrentHealth <= 0.0f)
	{
		OnPlayerDied.Broadcast();  // Death screen, ragdoll, kill feed
	}
}
```

### Pattern 3: BlueprintNativeEvent (C++ default + BP override)

Use when C++ has a sensible default behavior but designers may want to extend or replace it.

```cpp
// Header
/** Called when landing from a fall. Override in BP to add camera shake, sound, etc. */
UFUNCTION(BlueprintNativeEvent, Category = "Movement|Feedback")
void OnLanded(float ImpactVelocity);

// Source — C++ provides a default implementation
void ANPCharacterPawn::OnLanded_Implementation(float ImpactVelocity)
{
	// Default C++ behavior — apply landing stun if velocity exceeds threshold
	if (ImpactVelocity > HardLandingThreshold)
	{
		ApplyLandingStun(ImpactVelocity);
	}
	// BP override can call Super + add camera shake / sound / screen effect
}
```

---

## Animation Blueprint Integration

Animation Blueprints read C++ state — they never write gameplay state.

### Exposing State for AnimBP

```cpp
// In character or anim-specific component
UPROPERTY(BlueprintReadOnly, Category = "Animation")
float CurrentSpeed;

UPROPERTY(BlueprintReadOnly, Category = "Animation")
float MovementDirection;  // -180 to 180

UPROPERTY(BlueprintReadOnly, Category = "Animation")
bool bIsInAir;

UPROPERTY(BlueprintReadOnly, Category = "Animation")
bool bIsCrouching;

UPROPERTY(BlueprintReadOnly, Category = "Animation")
bool bIsSprinting;

UPROPERTY(BlueprintReadOnly, Category = "Animation")
ENPMovementState MovementState;

UPROPERTY(BlueprintReadOnly, Category = "Animation")
float AimPitch;  // Look up/down for aim offset

UPROPERTY(BlueprintReadOnly, Category = "Animation")
float AimYaw;    // Look left/right for aim offset

UPROPERTY(BlueprintReadOnly, Category = "Animation")
float LeanAngle; // Wallrun/turn lean
```

### Animation Montage Playback from C++

When C++ gameplay logic needs to trigger a montage, use a UPROPERTY for the montage asset:

```cpp
// Header — expose montage asset to BP
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
TObjectPtr<UAnimMontage> DodgeMontage;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
TObjectPtr<UAnimMontage> MantleMontage;

// Source — C++ decides WHEN to play, BP decides WHAT plays
void UNPDodgeLayeredMove::PlayDodgeAnim()
{
	if (ANPCharacterPawn* Pawn = Cast<ANPCharacterPawn>(GetOwner()))
	{
		if (UAnimInstance* AnimInst = Pawn->GetMesh()->GetAnimInstance())
		{
			if (IsValid(DodgeMontage))
			{
				AnimInst->Montage_Play(DodgeMontage, 1.0f);
			}
		}
	}
}
```

---

## UI / UMG Widget Integration

### Widget Class References

```cpp
// Expose widget class — BP_HUDWidget is created in UMG editor
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
TSubclassOf<UUserWidget> HUDWidgetClass;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
TSubclassOf<UUserWidget> ScoreboardWidgetClass;
```

### Creating Widgets from C++

```cpp
void ANPPlayerController::CreateHUD()
{
	if (!IsLocalController() || !HUDWidgetClass)
	{
		return;
	}

	HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}
```

### Data Binding — Expose State for Widgets

```cpp
// C++ component exposes state via delegates
UPROPERTY(BlueprintAssignable, Category = "UI|Binding")
FOnHealthChanged OnHealthChanged;

UPROPERTY(BlueprintAssignable, Category = "UI|Binding")
FOnAmmoChanged OnAmmoChanged;

// Widget BP binds to these delegates in its Event Graph or uses property binding
```

---

## Data-Driven Design

### Data Assets

```cpp
// Define a data asset class in C++
UCLASS(BlueprintType)
class NETSPER_API UNPWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float BaseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float FireRate = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 MagazineSize = 30;

	// Asset references — filled in editor
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Assets")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Assets")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Assets")
	TObjectPtr<UNiagaraSystem> MuzzleFlashVFX;
};
```

```cpp
// C++ reads the data asset at runtime — asset is assigned in BP/editor
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
TObjectPtr<UNPWeaponDataAsset> WeaponData;
```

### Data Tables

```cpp
// Define the row struct in C++
USTRUCT(BlueprintType)
struct FNPAbilityTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	float Cooldown = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	float StaminaCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	float Duration = 5.0f;
};
```

```cpp
// Expose DataTable reference — designers create/fill the table in editor
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
TObjectPtr<UDataTable> AbilityDataTable;

// Query at runtime
if (IsValid(AbilityDataTable))
{
	if (const FNPAbilityTableRow* Row = AbilityDataTable->FindRow<FNPAbilityTableRow>(RowName, TEXT("AbilityLookup")))
	{
		Cooldown = Row->Cooldown;
	}
}
```

---

## Tuning Parameters — EditDefaultsOnly

Expose all gameplay-tuning numbers so designers can iterate without recompiling:

```cpp
// Movement tuning
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
float MaxWalkSpeed = 600.0f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
float MaxSprintSpeed = 950.0f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning", meta = (ClampMin = "0.0", ClampMax = "3.0"))
float GravityScale = 1.8f;

// Stamina tuning
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Tuning")
float MaxStamina = 100.0f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Tuning")
float BaseRegenRate = 18.0f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina|Tuning")
float RegenDelay = 1.2f;

// Ability tuning
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Tuning")
float GrappleRange = 2000.0f;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Tuning")
float GrappleCooldown = 8.0f;
```

---

## Camera Effects

Camera effects (shakes, FOV changes, post-process) are cosmetic — BP handles the specifics:

```cpp
// Expose camera shake class — designer picks the shake asset
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
TSubclassOf<UCameraShakeBase> LandingCameraShake;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Shake")
TSubclassOf<UCameraShakeBase> DamageCameraShake;

// C++ triggers the shake
void ANPCharacterPawn::PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ShakeClass)
		{
			PC->ClientStartCameraShake(ShakeClass, Scale);
		}
	}
}
```

---

## Summary Decision Table

| Feature | Implement in C++? | Expose to BP? | How? |
|---------|-------------------|---------------|------|
| Movement physics | Yes | Read-only state | `BlueprintReadOnly` properties |
| Movement speeds | Yes (logic) | Tuning values | `EditDefaultsOnly` properties |
| Ability activation | Yes | Events | `BlueprintImplementableEvent` |
| Ability cooldowns | Yes | Tunable | `EditDefaultsOnly` floats |
| Damage calculation | Yes | Events | `BlueprintAssignable` delegates |
| Hit detection | Yes | Hit events | `BlueprintImplementableEvent` |
| Sound playback | No | Yes (fully BP) | `BlueprintImplementableEvent` hook |
| VFX spawning | No | Yes (fully BP) | Delegates or `BlueprintImplementableEvent` |
| Camera shake | Trigger only | Shake class | `TSubclassOf<UCameraShakeBase>` |
| UI updates | Data only | Widget class | Delegates + `TSubclassOf<UUserWidget>` |
| Animation state | Write state | Read state | `BlueprintReadOnly` + AnimBP reads |
| Montage trigger | When to play | What to play | `TObjectPtr<UAnimMontage>` property |
| Mesh/material | Never | Always | `TObjectPtr` / `TSoftObjectPtr` |
| Input bindings | Processing | Action assets | `TObjectPtr<UInputAction>` |
