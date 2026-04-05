// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Movement/NPMoverTypes.h"
#include "UObject/Interface.h"
#include "NPMovementController.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UNPMovementController : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class NETSPER_API INPMovementController
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void SetMoveDirection(const FVector2D& Direction) PURE_VIRTUAL(INPMovementController::SetMoveDirection);
	virtual void SetLookDirection(const FVector2D& Direction) PURE_VIRTUAL(INPMovementController::SetLookDirection);
	virtual void SetWantsToJump(bool bPressed) PURE_VIRTUAL(INPMovementController::SetJumpPressed);
	virtual void SetWantsToSprint(bool bPressed) PURE_VIRTUAL(INPMovementController::SetWantsToSprint);
	virtual void SetWantsToCrouch(bool bPressed) PURE_VIRTUAL(INPMovementController::SetWantsToCrouch);
	virtual void SetWantsToDodge(ENPDodgeDirection Direction) PURE_VIRTUAL(INPMovementController::SetWantsToDodge);
};
