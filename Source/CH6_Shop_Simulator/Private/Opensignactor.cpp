// Fill out your copyright notice in the Description page of Project Settings.

#include "OpenSignActor.h"
#include "GameTimeManager.h"
#include "Components/StaticMeshComponent.h"

AOpenSignActor::AOpenSignActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SignMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignMesh"));
	RootComponent = SignMesh;
}

void AOpenSignActor::TryOpen()
{
	if (GameTimeManager)
	{
		GameTimeManager->OpenShop();
	}
}