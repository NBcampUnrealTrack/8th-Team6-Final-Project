// Fill out your copyright notice in the Description page of Project Settings.

#include "ShopBuildStation.h"
#include "ShopBuildWidget.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"

AShopBuildStation::AShopBuildStation()
{
	PrimaryActorTick.bCanEverTick = false;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	RootComponent = StationMesh;
}

void AShopBuildStation::TryOpen(ACH6_Shop_SimulatorCharacter* InteractingCharacter)
{
	if (!BuildWidgetClass || !InteractingCharacter)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InteractingCharacter->GetController());
	if (!PC)
	{
		return;
	}

	ActiveWidget = CreateWidget<UShopBuildWidget>(PC, BuildWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->Setup(InteractingCharacter, this);
		ActiveWidget->AddToViewport();

		InteractingCharacter->bIsInteractionLocked = true;

		// ★ GameAndUI가 아니라 UIOnly로. GameAndUI는 UI 버튼 클릭이 "게임 쪽
		// 좌클릭 입력"으로도 동시에 전달돼서, 버튼 누르는 순간 그 클릭이
		// 캐릭터의 OnInteractPressed까지 같이 실행시켜버리는 문제가 있었음
		// (구매하자마자 그 자리에서 바로 확정 배치가 되어버림). UI만 입력을
		// 받게 하면 이 문제가 없어짐. 메뉴 닫힐 때 GameOnly로 다시 복귀시킴.
		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}