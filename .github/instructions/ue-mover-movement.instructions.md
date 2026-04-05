---
description: "Use when implementing movement modes, layered moves, mode transitions, or any Mover 2.0 / MoverComponent feature. Covers UBaseMovementMode, layered move system, mode registration, transition logic, input production via IMoverInputProducerInterface, and Mover-specific simulation patterns."
applyTo: "Source/**/*.h, Source/**/*.cpp"
---

# Mover 2.0 Movement System

## Architecture

Mover 2.0 is UE5's component-based movement framework replacing `UCharacterMovementComponent`. It uses:

- **`UMoverComponent`** — The movement simulation driver. Registered with NPP for networked prediction.
- **`UBaseMovementMode`** — Base class for movement modes (Ground, Air, Slide, etc.). Each mode owns its simulation logic.
- **`FLayeredMove`** / Layered Move classes — Temporary movement impulses layered on top of the active mode (Jump, Dodge, Landing Roll).
- **`IMoverInputProducerInterface`** — Interface the owning pawn implements to feed input to the Mover.
- **Movement Mixins** — Shared utility logic (gravity, floor detection) that modes can compose.

---

## MoverComponent Setup

```cpp
// In ANPCharacterPawn constructor
MoverComponent = CreateDefaultSubobject<UMoverComponent>(TEXT("MoverComponent"));

// MoverComponent configuration
MoverComponent->SetUpdatedComponent(CapsuleComp);
MoverComponent->SetIsReplicated(true);  // NPP handles replication

// Register movement modes
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPGroundMovementMode>(TEXT("GroundMode")));
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPAirMovementMode>(TEXT("AirMode")));
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPSlideMode>(TEXT("SlideMode")));
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPWallRunMode>(TEXT("WallRunMode")));
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPWallClimbMode>(TEXT("WallClimbMode")));
MoverComponent->AddMovementMode(CreateDefaultSubobject<UNPMantleMode>(TEXT("MantleMode")));

// Set default mode
MoverComponent->SetDefaultMovementMode(TEXT("GroundMode"));
```

## IMoverInputProducerInterface

The pawn implements this interface so MoverComponent can pull input each tick:

```cpp
// In ANPCharacterPawn header
UCLASS()
class NETSPER_API ANPCharacterPawn : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	// IMoverInputProducerInterface
	virtual void ProduceInput(int32 SimTimeMs, FMoverTickStartData& StartData) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPMovementInputComponent> MovementInputComponent;
};

// Source
void ANPCharacterPawn::ProduceInput(int32 SimTimeMs, FMoverTickStartData& StartData)
{
	// Delegate to the input component which accumulates Enhanced Input state
	if (IsValid(MovementInputComponent))
	{
		MovementInputComponent->ProduceInput(SimTimeMs, StartData);
	}
}
```

---

## Movement Mode Implementation

### Mode Base Pattern

```cpp
UCLASS()
class NETSPER_API UNPGroundMovementMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:
	virtual void OnActivated() override;
	virtual void OnDeactivated() override;
	virtual void SimulationTick(const FSimulationTickParams& Params, FSimulationOutput& Output) override;

protected:
	/** Compute velocity from input and current state. */
	FVector ComputeDesiredVelocity(const FNPMoverInputCmd& Input, const FNPMoverSyncState& State, float DeltaSeconds) const;

	/** Apply ground friction and acceleration. */
	FVector ApplyGroundPhysics(const FVector& CurrentVelocity, const FVector& DesiredVelocity, float DeltaSeconds) const;

	/** Check if we should leave ground mode. */
	bool ShouldTransitionToAir(const FNPMoverSyncState& State) const;
	bool ShouldTransitionToSlide(const FNPMoverInputCmd& Input, const FNPMoverSyncState& State) const;

	// Tuning — exposed so BP subclass can override
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxWalkSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float MaxSprintSpeed = 950.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float GroundFriction = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float GroundAcceleration = 2048.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ground|Tuning")
	float BrakingDeceleration = 2048.0f;
};
```

### SimulationTick Skeleton

Every movement mode follows this pattern:

```cpp
void UNPGroundMovementMode::SimulationTick(const FSimulationTickParams& Params, FSimulationOutput& Output)
{
	// === 1. Read Input & State ===
	const FNPMoverInputCmd& Input = Params.GetTypedInputCmd<FNPMoverInputCmd>();
	FNPMoverSyncState& Sync = Params.GetMutableTypedSyncState<FNPMoverSyncState>();
	const float DT = Params.DeltaSeconds;

	// === 2. Compute Movement ===
	const float TargetSpeed = Input.bWantsSprint ? MaxSprintSpeed : MaxWalkSpeed;
	FVector DesiredDir = FRotationMatrix(Input.ViewRotation).GetScaledAxis(EAxis::X) * Input.MoveInput.Y
	                   + FRotationMatrix(Input.ViewRotation).GetScaledAxis(EAxis::Y) * Input.MoveInput.X;
	DesiredDir.Z = 0.0f;
	DesiredDir = DesiredDir.GetClampedToMaxSize(1.0f) * TargetSpeed;

	// === 3. Apply Physics ===
	FVector NewVelocity = FMath::VInterpTo(Sync.Velocity, DesiredDir, DT, GroundAcceleration / TargetSpeed);
	NewVelocity.Z = 0.0f;  // Ground mode: no vertical velocity

	// === 4. Move ===
	const FVector Delta = NewVelocity * DT;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.bBlockingHit)
	{
		SlideAlongSurface(Delta, 1.0f - Hit.Time, Hit.Normal, Hit);
	}

	// === 5. Floor Check ===
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult);

	// === 6. Write State ===
	Sync.Location = UpdatedComponent->GetComponentLocation();
	Sync.Velocity = NewVelocity;
	Sync.MovementState = ENPMovementState::Ground;

	// === 7. Evaluate Transitions ===
	if (!FloorResult.bWalkableFloor)
	{
		Output.RequestModeTransition(TEXT("AirMode"));
	}
	else if (ShouldTransitionToSlide(Input, Sync))
	{
		Output.RequestModeTransition(TEXT("SlideMode"));
	}
}
```

### Mode Lifecycle

```cpp
void UNPGroundMovementMode::OnActivated()
{
	Super::OnActivated();
	// Reset mode-specific state
	// DO NOT play cosmetic effects here — use cues or BlueprintImplementableEvent
}

void UNPGroundMovementMode::OnDeactivated()
{
	Super::OnDeactivated();
	// Clean up mode-specific state
}
```

---

## Mode Transitions

Transitions are requested from within `SimulationTick` or from the MoverComponent:

```cpp
// From within SimulationTick — request transition
Output.RequestModeTransition(TEXT("AirMode"));

// Transition conditions must be deterministic (same on client and server)
bool UNPGroundMovementMode::ShouldTransitionToSlide(const FNPMoverInputCmd& Input, const FNPMoverSyncState& Sync) const
{
	// Sprint + crouch + speed threshold = slide
	return Input.bWantsSprint
		&& Input.bWantsCrouch
		&& Sync.Velocity.Size2D() >= MinSlideEntrySpeed;
}
```

### Transition Decision Table

| From | To | Condition |
|------|----|-----------|
| Ground | Air | No walkable floor, or jump |
| Ground | Slide | Sprint + Crouch + speed ≥ threshold |
| Air | Ground | Landed on walkable floor |
| Air | WallRun | Wall trace hit + sufficient speed + angle check |
| Air | WallClimb | Wall trace hit + forward input + upward velocity |
| Air | Mantle | Ledge detected at chest height |
| Slide | Ground | Speed below threshold or release crouch |
| Slide | Air | Leave ground while sliding |
| WallRun | Air | Duration expired, jump off, or lose wall |
| WallClimb | Air | Duration expired or upward velocity exhausted |
| Mantle | Ground | Mantle animation complete, on top of ledge |

---

## Layered Moves

Layered moves are temporary impulses that blend with or override the active mode's output.

### Defining a Layered Move

```cpp
USTRUCT()
struct FNPJumpLayeredMove : public FLayeredMoveBase
{
	GENERATED_BODY()

	FNPJumpLayeredMove();

	/** The jump impulse velocity. */
	FVector JumpVelocity = FVector::ZeroVector;

	// FLayeredMoveBase interface
	virtual bool GenerateMove(const FMoverTickStartData& StartState,
		const FMoverTimeStep& TimeStep,
		const UMoverComponent* MoverComp,
		UMoverBlackboard* SimBlackboard,
		FProposedMove& OutProposedMove) override;

	virtual FLayeredMoveBase* Clone() const override;
	virtual void NetSerialize(FArchive& Ar) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FString ToSimpleString() const override;
};
```

### Generating the Move

```cpp
bool FNPJumpLayeredMove::GenerateMove(
	const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep,
	const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard,
	FProposedMove& OutProposedMove)
{
	// Apply jump impulse
	OutProposedMove.LinearVelocity = JumpVelocity;
	OutProposedMove.MixMode = EMoveMixMode::AdditiveVelocity; // Adds to mode velocity

	// One-shot: mark as finished after first application
	return false; // false = remove this layered move after this tick
}
```

### Queuing Layered Moves

```cpp
// Queue a jump layered move (from SimulationTick or input processing)
void QueueJump(UMoverComponent* MoverComp, const FVector& JumpVelocity)
{
	FNPJumpLayeredMove JumpMove;
	JumpMove.JumpVelocity = JumpVelocity;
	JumpMove.MixMode = EMoveMixMode::AdditiveVelocity;
	JumpMove.Duration = 0.0f; // Instant, single-tick

	MoverComp->QueueLayeredMove(JumpMove);
}

// Queue a dodge layered move
void QueueDodge(UMoverComponent* MoverComp, const FVector& DodgeDirection, float DodgeSpeed, float DodgeDuration)
{
	FNPDodgeLayeredMove DodgeMove;
	DodgeMove.DodgeVelocity = DodgeDirection * DodgeSpeed;
	DodgeMove.Duration = DodgeDuration;
	DodgeMove.MixMode = EMoveMixMode::OverrideVelocity;

	MoverComp->QueueLayeredMove(DodgeMove);
}
```

---

## Stamina Integration in Movement Modes

Movement modes query stamina via the `INPStaminaProvider` interface, NOT by holding a direct reference to the stamina component:

```cpp
void UNPGroundMovementMode::SimulationTick(const FSimulationTickParams& Params, FSimulationOutput& Output)
{
	const FNPMoverInputCmd& Input = Params.GetTypedInputCmd<FNPMoverInputCmd>();
	FNPMoverSyncState& Sync = Params.GetMutableTypedSyncState<FNPMoverSyncState>();
	const float DT = Params.DeltaSeconds;

	// Query stamina through interface
	bool bCanSprint = false;
	if (Input.bWantsSprint)
	{
		if (INPStaminaProvider* StaminaProvider = Cast<INPStaminaProvider>(GetOwner()))
		{
			bCanSprint = StaminaProvider->TryConsumeStamina(SprintStaminaCostPerSecond * DT);
		}
	}

	const float TargetSpeed = bCanSprint ? MaxSprintSpeed : MaxWalkSpeed;
	// ... rest of movement logic
}
```

---

## Floor and Wall Detection

### Floor Finding

```cpp
void FindFloor(const FVector& Location, FFindFloorResult& OutResult) const
{
	FHitResult Hit;
	const FVector TraceStart = Location;
	const FVector TraceEnd = Location - FVector(0.0f, 0.0f, FloorTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutResult.bWalkableFloor = Hit.Normal.Z >= WalkableFloorZ;
		OutResult.FloorDist = Hit.Distance;
		OutResult.HitResult = Hit;
	}
}
```

### Wall Detection (for WallRun/WallClimb)

```cpp
bool TraceForWall(const FVector& Location, const FVector& Velocity, FHitResult& OutHit) const
{
	// Trace perpendicular to movement direction to find walls
	const FVector MoveDir2D = FVector(Velocity.X, Velocity.Y, 0.0f).GetSafeNormal();
	const FVector RightDir = FVector::CrossProduct(MoveDir2D, FVector::UpVector);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	// Trace both sides
	bool bHitLeft = GetWorld()->LineTraceSingleByChannel(OutHit, Location, Location - RightDir * WallTraceDistance, ECC_Visibility, QueryParams);
	if (bHitLeft && IsWallRunnable(OutHit.Normal))
	{
		return true;
	}

	bool bHitRight = GetWorld()->LineTraceSingleByChannel(OutHit, Location, Location + RightDir * WallTraceDistance, ECC_Visibility, QueryParams);
	if (bHitRight && IsWallRunnable(OutHit.Normal))
	{
		return true;
	}

	return false;
}
```

---

## Movement Cues for Cosmetics

Use cues (via AuxState flags or dedicated cue system) to trigger cosmetic events without polluting SimulationTick:

```cpp
// In AuxState — server sets, client reads for cosmetics
USTRUCT()
struct FNPMoverAuxState
{
	GENERATED_BODY()

	bool bCue_Jumped = false;
	bool bCue_Landed = false;
	bool bCue_SlideStarted = false;
	bool bCue_WallRunStarted = false;
	bool bCue_DodgePerformed = false;
	float LandingImpactSpeed = 0.0f;
};

// Post-simulation — consume cues and fire events
void ANPCharacterPawn::PostSimulationTick()
{
	const FNPMoverAuxState& Aux = MoverComponent->GetAuxState<FNPMoverAuxState>();

	if (Aux.bCue_Jumped)
	{
		OnJumpPerformed();  // BlueprintImplementableEvent → BP plays jump VFX/SFX
	}
	if (Aux.bCue_Landed)
	{
		OnLanded(Aux.LandingImpactSpeed);  // BlueprintNativeEvent
	}
	if (Aux.bCue_WallRunStarted)
	{
		OnWallRunStarted(bIsLeftWall);  // BlueprintImplementableEvent
	}
}
```
