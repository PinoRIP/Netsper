#include "Input/NPMovementInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MoverDataModelTypes.h"
#include "Movement/NPMoverTypes.h"
#include "GameFramework/PlayerController.h"
#include "Netsper.h"

UNPMovementInputComponent::UNPMovementInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPMovementInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// Add mapping context
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn))
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!IsValid(PC))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (!IsValid(InputSubsystem))
	{
		return;
	}

	if (IMC_OnFoot)
	{
		InputSubsystem->AddMappingContext(IMC_OnFoot, 0);
	}

	if (IMC_Ability)
	{
		InputSubsystem->AddMappingContext(IMC_Ability, 1);
	}
}

void UNPMovementInputComponent::SetupInputBindings(UEnhancedInputComponent* EIC)
{
	if (!IsValid(EIC))
	{
		return;
	}

	if (IsValid(IA_Move))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &UNPMovementInputComponent::OnMoveTriggered);
		EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnMoveCompleted);
	}

	if (IsValid(IA_Look))
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &UNPMovementInputComponent::OnLookTriggered);
	}

	if (IsValid(IA_Jump))
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnJumpStarted);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnJumpReleased);
	}

	if (IsValid(IA_Sprint))
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnSprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnSprintReleased);
	}

	if (IsValid(IA_Crouch))
	{
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnCrouchStarted);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &UNPMovementInputComponent::OnCrouchReleased);
	}

	if (IsValid(IA_Dodge))
	{
		EIC->BindAction(IA_Dodge, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnDodgeStarted);
	}

	if (IsValid(IA_Mantle))
	{
		EIC->BindAction(IA_Mantle, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnMantleStarted);
	}

	if (IsValid(IA_Ability))
	{
		EIC->BindAction(IA_Ability, ETriggerEvent::Started, this, &UNPMovementInputComponent::OnAbilityStarted);
	}
}

void UNPMovementInputComponent::ProduceInput(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn))
	{
		return;
	}

	// Build default character inputs (consumed by standard Mover modes)
	FCharacterDefaultInputs& DefaultInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	// Compute world-space move direction from camera-relative input
	const FRotator ControlRot = OwnerPawn->GetControlRotation();
	const FRotator YawOnlyRot(0.f, ControlRot.Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::X);
	const FVector RightDir = FRotationMatrix(YawOnlyRot).GetUnitAxis(EAxis::Y);

	FVector MoveDirection = ForwardDir * CachedMoveInput.Y + RightDir * CachedMoveInput.X;
	if (!MoveDirection.IsNearlyZero())
	{
		MoveDirection.Normalize();
	}

	DefaultInputs.SetMoveInput(EMoveInputType::DirectionalIntent, MoveDirection);
	DefaultInputs.ControlRotation = ControlRot;
	DefaultInputs.OrientationIntent = ForwardDir;
	DefaultInputs.bIsJumpJustPressed = bJumpPressed;
	DefaultInputs.bIsJumpPressed = bJumpHeld;

	// Build custom NP inputs
	FNPMoverInputCmd& NPInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FNPMoverInputCmd>();
	NPInputs.bWantsSprint = bSprintHeld;
	NPInputs.bWantsCrouch = bCrouchHeld;
	NPInputs.bWantsDodge = bDodgePressed;
	NPInputs.bWantsMantle = bMantlePressed;
	NPInputs.bWantsAbility = bAbilityPressed;

	// Clear one-shot flags (consumed per sim tick, not per frame)
	bJumpPressed = false;
	bDodgePressed = false;
	bMantlePressed = false;
	bAbilityPressed = false;
}

// Input callbacks

void UNPMovementInputComponent::OnMoveTriggered(const FInputActionValue& Value)
{
	CachedMoveInput = Value.Get<FVector2D>();
}

void UNPMovementInputComponent::OnMoveCompleted(const FInputActionValue& Value)
{
	CachedMoveInput = FVector2D::ZeroVector;
}

void UNPMovementInputComponent::OnLookTriggered(const FInputActionValue& Value)
{
	CachedLookDelta = Value.Get<FVector2D>();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn))
	{
		OwnerPawn->AddControllerYawInput(CachedLookDelta.X);
		OwnerPawn->AddControllerPitchInput(CachedLookDelta.Y);
	}
}

void UNPMovementInputComponent::OnJumpStarted(const FInputActionValue& Value)
{
	bJumpPressed = true;
	bJumpHeld = true;
}

void UNPMovementInputComponent::OnJumpReleased(const FInputActionValue& Value)
{
	bJumpHeld = false;
}

void UNPMovementInputComponent::OnSprintStarted(const FInputActionValue& Value)
{
	bSprintHeld = true;
}

void UNPMovementInputComponent::OnSprintReleased(const FInputActionValue& Value)
{
	bSprintHeld = false;
}

void UNPMovementInputComponent::OnCrouchStarted(const FInputActionValue& Value)
{
	bCrouchHeld = true;
}

void UNPMovementInputComponent::OnCrouchReleased(const FInputActionValue& Value)
{
	bCrouchHeld = false;
}

void UNPMovementInputComponent::OnDodgeStarted(const FInputActionValue& Value)
{
	bDodgePressed = true;
}

void UNPMovementInputComponent::OnMantleStarted(const FInputActionValue& Value)
{
	bMantlePressed = true;
}

void UNPMovementInputComponent::OnAbilityStarted(const FInputActionValue& Value)
{
	bAbilityPressed = true;
}
