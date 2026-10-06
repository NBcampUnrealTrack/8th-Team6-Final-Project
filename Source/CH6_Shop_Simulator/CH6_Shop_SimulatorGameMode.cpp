// Copyright Epic Games, Inc. All Rights Reserved.

#include "CH6_Shop_SimulatorGameMode.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "CustomerAI.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACH6_Shop_SimulatorGameMode::ACH6_Shop_SimulatorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void ACH6_Shop_SimulatorGameMode::BeginPlay()
{
	Super::BeginPlay();
}

