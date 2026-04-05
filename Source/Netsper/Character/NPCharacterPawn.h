#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MoverSimulationTypes.h"
#include "NPCharacterPawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UMoverComponent;
class UNPMovementInputComponent;
class UNPStaminaComponent;
class UNPAbilityComponent;
class UNPHealthComponent;
class UNPWeaponComponent;
class UNPCameraComponent;

/**
 * ANPCharacterPawn — Core gameplay pawn for Netsper.
 *
 * Thin orchestrator: owns the component hierarchy,
 * delegates to components for all logic.
 * Uses Mover 2.0 (NPP backend) for movement.
 */
UCLASS()
class NETSPER_API ANPCharacterPawn : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	ANPCharacterPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IMoverInputProducerInterface
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	// APawn overrides
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual FVector GetPawnViewLocation() const override;
	virtual FRotator GetViewRotation() const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Component accessors
	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UMoverComponent* GetMoverComponent() const { return MoverComponent; }

	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UNPStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UNPAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UNPHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UNPWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	UFUNCTION(BlueprintCallable, Category = "NP|Components")
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComp; }

protected:
	virtual void BeginPlay() override;

	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	// Meshes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> ArmsMesh;

	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoverComponent> MoverComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPMovementInputComponent> MovementInputComponent;

	// Gameplay
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPAbilityComponent> AbilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPHealthComponent> HealthComponent;

	// Combat
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPWeaponComponent> WeaponComponent;

	// Camera effects
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNPCameraComponent> CameraEffectsComponent;
};
