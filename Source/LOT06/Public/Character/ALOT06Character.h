#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ALOT06Character.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class LOT06_API AALOT06Character : public ACharacter
{
	GENERATED_BODY()

public:
	AALOT06Character();

protected:
	virtual void BeginPlay() override;
	virtual void PawnClientRestart() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionDistance = 300.0f;

	void Look(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void Interact();
};
