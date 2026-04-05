#include "Input/NPMovementInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Movement/NPMoverTypes.h"
#include "GameFramework/PlayerController.h"
#include "Movement/Input/NPMovementController.h"

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
}


TScriptInterface<INPMovementController> UNPMovementInputComponent::GetMovementController()
{
	if (!MovementController)
	{
		UActorComponent* Component = GetOwner()->FindComponentByInterface(INPMovementController::UClassType::StaticClass());
		if (INPMovementController* InterfacePointer = Cast<INPMovementController>(Component))
		{
			MovementController = TScriptInterface<INPMovementController>();
			MovementController.SetInterface(InterfacePointer);
			MovementController.SetObject(Component);
		}
	}
	
	return MovementController;
}

void UNPMovementInputComponent::OnMoveTriggered(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetMoveDirection(Value.Get<FVector2D>());
	}
}

void UNPMovementInputComponent::OnMoveCompleted(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetMoveDirection(FVector2D::ZeroVector);
	}
}

void UNPMovementInputComponent::OnLookTriggered(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetLookDirection(Value.Get<FVector2D>());
	}
}

void UNPMovementInputComponent::OnJumpStarted(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToJump(true);
	}
}

void UNPMovementInputComponent::OnJumpReleased(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToJump(false);
	}
}

void UNPMovementInputComponent::OnSprintStarted(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToSprint(true);
	}
}

void UNPMovementInputComponent::OnSprintReleased(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToSprint(false);
	}
}

void UNPMovementInputComponent::OnCrouchStarted(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToCrouch(true);
	}
}

void UNPMovementInputComponent::OnCrouchReleased(const FInputActionValue& Value)
{
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToCrouch(false);
	}
}

void UNPMovementInputComponent::OnDodgeStarted(const FInputActionValue& Value)
{
	// TODO: Dodging directions
	if (TScriptInterface<INPMovementController> Controller = GetMovementController())
	{
		Controller->SetWantsToDodge(ENPDodgeDirection::Left);
	}
}