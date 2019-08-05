// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TheBorderCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"
#include "BorderSaveGame.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"

//////////////////////////////////////////////////////////////////////////
// ABorderCharacter

ATheBorderCharacter::ATheBorderCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 120.0f);

	SpeedSprint = 5;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 60.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxAcceleration = 600.f;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 300.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	//CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//CameraBoom->SetupAttachment(RootComponent);
	//CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	//CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->AttachTo(GetMesh());
	FollowCamera->bUsePawnControlRotation = true; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATheBorderCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	//PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jumping);
	//PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATheBorderCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATheBorderCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ATheBorderCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ATheBorderCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATheBorderCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATheBorderCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ATheBorderCharacter::OnResetVR);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ATheBorderCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ATheBorderCharacter::StopSprint);

	PlayerInputComponent->BindAction("Save", IE_Pressed, this, &ATheBorderCharacter::SaveGame);
	PlayerInputComponent->BindAction("Load", IE_Pressed, this, &ATheBorderCharacter::LoadGame);

	PlayerInputComponent->BindAction("Action", IE_Pressed, this, &ATheBorderCharacter::OnAction);

	CurrentDoor = NULL;
}

void ATheBorderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FHitResult Hit;
	FVector Start = FollowCamera->GetComponentLocation();
	FVector ForwardVector = FollowCamera->GetForwardVector();
	FVector End = (ForwardVector * 200.f) + Start;
	FCollisionQueryParams CollisionParams;

	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1, 0, 1);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionParams))
	{
		if (Hit.bBlockingHit)
		{
			if (Hit.GetActor()->GetClass()->IsChildOf(ASwingDoor::StaticClass()))
			{
				CurrentDoor = Cast<ASwingDoor>(Hit.GetActor());
			}
		}
	}
	else
	{
		CurrentDoor = NULL;
	}
}


void ATheBorderCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ATheBorderCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void ATheBorderCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ATheBorderCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ATheBorderCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ATheBorderCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ATheBorderCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ATheBorderCharacter::Sprint()
{
	GetCharacterMovement()->MaxWalkSpeed *= SpeedSprint;
}

void ATheBorderCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed /= SpeedSprint;
}

void ATheBorderCharacter::OnAction()
{
	if (CurrentDoor)
	{
		CurrentDoor->ToggleDoor();
	}
}
//!!!Что то не сохраняет поворот в SaveGameInstance
void ATheBorderCharacter::SaveGame()
{
	UBorderSaveGame* SaveGameInstance = Cast<UBorderSaveGame>(UGameplayStatics::CreateSaveGameObject(UBorderSaveGame::StaticClass()));
	//Put into SaveGameInstance Statics
	SaveGameInstance->PlayerLocation = this->GetActorLocation();
	SaveGameInstance->PlayerRotation = this->GetActorRotation();
	//Save the SaveGameInstance
	UGameplayStatics::SaveGameToSlot(SaveGameInstance,TEXT("SlotGame"),0);
	//Log a message
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Game Saved"));
}

void ATheBorderCharacter::LoadGame()
{
	UBorderSaveGame* SaveGameInstance = Cast<UBorderSaveGame>(UGameplayStatics::CreateSaveGameObject(UBorderSaveGame::StaticClass()));
	//load a saved game into savegame instance variable
	SaveGameInstance = Cast<UBorderSaveGame>(UGameplayStatics::LoadGameFromSlot("SlotGame", 0));
	this->SetActorLocation(SaveGameInstance->PlayerLocation);
	this->SetActorRotation(SaveGameInstance->PlayerRotation);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Game Loaded"));
}
