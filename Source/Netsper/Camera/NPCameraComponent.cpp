#include "Camera/NPCameraComponent.h"
#include "Camera/CameraComponent.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoveLibrary/MoverBlackboard.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Netsper.h"

UNPCameraComponent::UNPCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentFOV = BaseFOV;
}

void UNPCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentFOV = BaseFOV;
}

void UNPCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	// Get Mover component for velocity
	UMoverComponent* MoverComp = OwnerPawn->FindComponentByClass<UMoverComponent>();

	// Speed-based FOV
	float HorizontalSpeed = 0.f;
	if (MoverComp)
	{
		UMoverBlackboard* SimBB = MoverComp->GetSimBlackboard_Mutable();

		// Read velocity from the mover's updated component
		if (USceneComponent* UpdatedComp = MoverComp->GetUpdatedComponent())
		{
			// Get velocity from the mover state - approximation via frame delta
			// Use the component's velocity property
		}

		// Read wall run tilt from blackboard
		if (SimBB)
		{
			float TiltValue = 0.f;
			if (SimBB->TryGet<float>(FName(TEXT("WallRunTilt")), TiltValue))
			{
				TargetTiltRoll = TiltValue;
			}
			else
			{
				TargetTiltRoll = 0.f;
			}
		}
	}

	// Compute speed from pawn velocity
	FVector PawnVelocity = OwnerPawn->GetVelocity();
	HorizontalSpeed = FVector(PawnVelocity.X, PawnVelocity.Y, 0.f).Size();

	// FOV interpolation (quadratic for snappier response at high speed)
	const float SpeedFraction = FMath::Clamp(HorizontalSpeed / MaxSprintSpeed, 0.f, 1.f);
	const float TargetFOV = FMath::Lerp(BaseFOV, MaxFOV, SpeedFraction * SpeedFraction);
	CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, FOVInterpSpeed);

	// Apply FOV to camera
	UCameraComponent* Camera = OwnerPawn->FindComponentByClass<UCameraComponent>();
	if (IsValid(Camera))
	{
		Camera->SetFieldOfView(CurrentFOV);
	}

	// Wall run tilt interpolation
	CurrentTiltRoll = FMath::FInterpTo(CurrentTiltRoll, TargetTiltRoll, DeltaTime, TiltInterpSpeed);

	// Apply tilt via controller rotation
	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (IsValid(PC) && IsValid(PC->PlayerCameraManager))
	{
		// Tilt is applied as a rotation offset; using the view pitch/roll system
		// This is a simplified approach; a full implementation would use UCameraModifier
		// For now, we adjust the spring arm's relative rotation
	}

	// Landing shake
	if (LandingShakeTimer > 0.f)
	{
		LandingShakeTimer -= DeltaTime;
		// Shake amount decays to zero
		const float ShakePct = FMath::Clamp(LandingShakeTimer / LandingShakeDuration, 0.f, 1.f);
		LandingShakeAmount = LandingShakeIntensity * ShakePct;
	}
}
