#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoverSimulationTypes.h"
#include "NPMovementInputComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
struct FInputActionValue;

/**
 * UNPMovementInputComponent — Bridges Enhanced Input to Mover's input system.
 *
 * Accumulates input state from Enhanced Input callbacks and writes it into
 * the Mover input command during ProduceInput.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NETSPER_API UNPMovementInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPMovementInputComponent();

	/** Called by the pawn to set up Enhanced Input bindings */
	void SetupInputBindings(UEnhancedInputComponent* EIC);

	/** Called by ANPCharacterPawn::ProduceInput to fill the Mover input command */
	void ProduceInput(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult);

	// Input Actions (assigned in Blueprint/Editor)
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
	TObjectPtr<UInputAction> IA_Mantle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> IA_Ability;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> IA_PrimaryFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> IA_SecondaryFire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Actions")
	TObjectPtr<UInputAction> IA_WeaponSwitch;

	// Input Mapping Contexts
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> IMC_OnFoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Contexts")
	TObjectPtr<UInputMappingContext> IMC_Ability;

protected:
	virtual void BeginPlay() override;

private:
	// Enhanced Input callbacks — accumulate into frame cache
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);
	void OnLookTriggered(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnSprintStarted(const FInputActionValue& Value);
	void OnSprintReleased(const FInputActionValue& Value);
	void OnCrouchStarted(const FInputActionValue& Value);
	void OnCrouchReleased(const FInputActionValue& Value);
	void OnDodgeStarted(const FInputActionValue& Value);
	void OnMantleStarted(const FInputActionValue& Value);
	void OnAbilityStarted(const FInputActionValue& Value);

	// Frame-local input cache (gathered at frame rate, consumed at sim rate)
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	FVector2D CachedLookDelta = FVector2D::ZeroVector;

	bool bJumpPressed = false;
	bool bJumpHeld = false;
	bool bSprintHeld = false;
	bool bCrouchHeld = false;
	bool bDodgePressed = false;
	bool bMantlePressed = false;
	bool bAbilityPressed = false;
};
