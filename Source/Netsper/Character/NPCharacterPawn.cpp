#include "Character/NPCharacterPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "MoverComponent.h"
#include "Input/NPMovementInputComponent.h"
#include "Stamina/NPStaminaComponent.h"
#include "Abilities/NPAbilityComponent.h"
#include "Health/NPHealthComponent.h"
#include "Weapons/NPWeaponComponent.h"
#include "Camera/NPCameraComponent.h"
#include "Movement/NPMoverTypes.h"
#include "Movement/Modes/NPGroundMovementMode.h"
#include "Movement/Modes/NPAirMovementMode.h"
#include "Movement/Modes/NPSlideMode.h"
#include "Movement/Modes/NPWallRunMode.h"
#include "Movement/Modes/NPWallClimbMode.h"
#include "Movement/Modes/NPMantleMode.h"
#include "Net/UnrealNetwork.h"
#include "Netsper.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ANPCharacterPawn::ANPCharacterPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Replication
	bReplicates = true;
	bAlwaysRelevant = false;
	SetNetUpdateFrequency(60.f);
	SetMinNetUpdateFrequency(15.f);
	SetNetCullDistanceSquared(22500.f * 22500.f);

	// Capsule (root)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 48.f); // radius 42, half-height 48 (total 96cm)
	CapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SetRootComponent(CapsuleComp);

	// Body mesh (third-person / shadow)
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(CapsuleComp);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -48.f));
	BodyMesh->bOwnerNoSee = true;
	BodyMesh->bCastDynamicShadow = true;
	BodyMesh->CastShadow = true;

	// Spring arm (zero length, at eye height)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CapsuleComp);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 32.f)); // Eye height +80 from capsule bottom = +32 from capsule center
	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = false;

	// Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);

	// Arms mesh (first-person, attached to camera)
	ArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsMesh"));
	ArmsMesh->SetupAttachment(FollowCamera);
	ArmsMesh->SetOnlyOwnerSee(true);
	ArmsMesh->bCastDynamicShadow = false;
	ArmsMesh->CastShadow = false;

	// Mover component
	MoverComponent = CreateDefaultSubobject<UMoverComponent>(TEXT("MoverComponent"));
	MoverComponent->SetUpdatedComponent(CapsuleComp);
	MoverComponent->SetIsReplicated(true);
	MoverComponent->StartingMovementMode = NPMovementModeNames::Ground;

	// NOTE: Movement modes are registered in PostInitializeComponents,
	// after MoverComponent::InitializeComponent has created ModeFSM.

	// Input component
	MovementInputComponent = CreateDefaultSubobject<UNPMovementInputComponent>(TEXT("MovementInputComponent"));

	// Stamina
	StaminaComponent = CreateDefaultSubobject<UNPStaminaComponent>(TEXT("StaminaComponent"));

	// Abilities
	AbilityComponent = CreateDefaultSubobject<UNPAbilityComponent>(TEXT("AbilityComponent"));

	// Health
	HealthComponent = CreateDefaultSubobject<UNPHealthComponent>(TEXT("HealthComponent"));

	// Weapon
	WeaponComponent = CreateDefaultSubobject<UNPWeaponComponent>(TEXT("WeaponComponent"));

	// Camera effects
	CameraEffectsComponent = CreateDefaultSubobject<UNPCameraComponent>(TEXT("CameraEffectsComponent"));
}

void ANPCharacterPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Register movement modes now that MoverComponent::InitializeComponent has run
	if (IsValid(MoverComponent))
	{
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::Ground, UNPGroundMovementMode::StaticClass());
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::Air, UNPAirMovementMode::StaticClass());
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::Slide, UNPSlideMode::StaticClass());
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::WallRun, UNPWallRunMode::StaticClass());
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::WallClimb, UNPWallClimbMode::StaticClass());
		MoverComponent->AddMovementModeFromClass(NPMovementModeNames::Mantle, UNPMantleMode::StaticClass());
	}
}

void ANPCharacterPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ANPCharacterPawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	if (IsValid(MovementInputComponent))
	{
		MovementInputComponent->ProduceInput(SimTimeMs, InputCmdResult);
	}
}

void ANPCharacterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (IsValid(MovementInputComponent))
	{
		UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		if (IsValid(EIC))
		{
			MovementInputComponent->SetupInputBindings(EIC);
		}
	}
}

FVector ANPCharacterPawn::GetPawnViewLocation() const
{
	if (IsValid(FollowCamera))
	{
		return FollowCamera->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

FRotator ANPCharacterPawn::GetViewRotation() const
{
	if (IsValid(Controller))
	{
		return Controller->GetControlRotation();
	}
	return Super::GetViewRotation();
}

void ANPCharacterPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
