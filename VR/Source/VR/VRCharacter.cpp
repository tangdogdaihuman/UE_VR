// Copyright Epic Games, Inc. All Rights Reserved.

#include "VRCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/InteractionManager.h"
#include "VR.h"

AVRCharacter::AVRCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Create runtime InputActions (no .uasset files needed)
	InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	Action1Action = NewObject<UInputAction>(this, TEXT("IA_Action1"));
	Action1Action->ValueType = EInputActionValueType::Boolean;

	Action2Action = NewObject<UInputAction>(this, TEXT("IA_Action2"));
	Action2Action->ValueType = EInputActionValueType::Boolean;

	Action3Action = NewObject<UInputAction>(this, TEXT("IA_Action3"));
	Action3Action->ValueType = EInputActionValueType::Boolean;
}

void AVRCharacter::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogVR, Log, TEXT("[VRCharacter] BeginPlay. InteractAction ready — no .uasset needed"));
}

void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AVRCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AVRCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVRCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVRCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AVRCharacter::LookInput);

		// Interact (mouse click / VR trigger) -> InteractionManager::HandleClick
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AVRCharacter::InteractInput);

		// Action keys (keyboard 1/2/3) — route to InteractionManager for quiz/scene interaction
		EnhancedInputComponent->BindAction(Action1Action, ETriggerEvent::Started, this, &AVRCharacter::HandleAction1);
		EnhancedInputComponent->BindAction(Action2Action, ETriggerEvent::Started, this, &AVRCharacter::HandleAction2);
		EnhancedInputComponent->BindAction(Action3Action, ETriggerEvent::Started, this, &AVRCharacter::HandleAction3);
	}
	else
	{
		UE_LOG(LogVR, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AVRCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AVRCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

	// Refresh ray trace on camera rotation (event-driven, not per-tick)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteractionManager* IM = GI->GetSubsystem<UInteractionManager>())
		{
			IM->RefreshTrace();
		}
	}
}

void AVRCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);

		// Trigger interaction ray refresh on view rotation (event-driven, not per-tick)
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UInteractionManager* InteractionMgr = GI->GetSubsystem<UInteractionManager>())
			{
				InteractionMgr->RefreshTrace();
			}
		}
	}
}

void AVRCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AVRCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AVRCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AVRCharacter::InteractInput(const FInputActionValue& Value)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteractionManager* IM = GI->GetSubsystem<UInteractionManager>())
		{
			IM->HandleClick();
			IM->RefreshTrace();
		}
	}
}

void AVRCharacter::HandleAction1(const FInputActionValue& Value)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteractionManager* IM = GI->GetSubsystem<UInteractionManager>())
		{
			IM->HandleActionKey(1);
		}
	}
}

void AVRCharacter::HandleAction2(const FInputActionValue& Value)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteractionManager* IM = GI->GetSubsystem<UInteractionManager>())
		{
			IM->HandleActionKey(2);
		}
	}
}

void AVRCharacter::HandleAction3(const FInputActionValue& Value)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteractionManager* IM = GI->GetSubsystem<UInteractionManager>())
		{
			IM->HandleActionKey(3);
		}
	}
}
