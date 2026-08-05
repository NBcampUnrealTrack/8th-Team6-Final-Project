#include "Character/ALOT06Character.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"

AALOT06Character::AALOT06Character()
{
 	
	PrimaryActorTick.bCanEverTick = false;
}

void AALOT06Character::BeginPlay()
{
	Super::BeginPlay();

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AALOT06Character::PawnClientRestart()
{
    Super::PawnClientRestart();

    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !DefaultMappingContext)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

    if (!InputSubsystem)
    {
        return;
    }

    InputSubsystem->RemoveMappingContext(DefaultMappingContext);
    InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
}

void AALOT06Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

//¿Œ«≤ πŸ¿Œµ˘
void AALOT06Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

    if (!EnhancedInput)
    {
        return;
    }

    EnhancedInput->BindAction(
        LookAction,
        ETriggerEvent::Triggered,
        this,
        &ThisClass::Look
    );

    EnhancedInput->BindAction(
        MoveAction,
        ETriggerEvent::Triggered,
        this,
        &ThisClass::Move
    );

    EnhancedInput->BindAction(
        SprintAction,
        ETriggerEvent::Started,
        this,
        &ThisClass::StartSprint
    );

    EnhancedInput->BindAction(
        SprintAction,
        ETriggerEvent::Completed,
        this,
        &ThisClass::StopSprint
    );

    EnhancedInput->BindAction(
        SprintAction,
        ETriggerEvent::Canceled,
        this,
        &ThisClass::StopSprint
    );
}

void AALOT06Character::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();

    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}

void AALOT06Character::Move(const FInputActionValue& Value)
{
    const FVector2D MoveAxis = Value.Get<FVector2D>();

    if (!Controller)
    {
        return;
    }

    const FRotator ControlRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

    const FVector ForwardDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

    const FVector RightDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MoveAxis.Y);
    AddMovementInput(RightDirection, MoveAxis.X);
}

void AALOT06Character::StartSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AALOT06Character::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}


