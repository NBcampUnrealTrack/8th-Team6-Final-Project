// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CH6_Shop_SimulatorGameMode.generated.h"

UCLASS(minimalapi)
class ACH6_Shop_SimulatorGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACH6_Shop_SimulatorGameMode();

protected:
	void BeginPlay();
};



