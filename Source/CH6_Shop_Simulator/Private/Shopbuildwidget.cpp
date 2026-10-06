// Fill out your copyright notice in the Description page of Project Settings.

#include "ShopBuildWidget.h"
#include "ShopBuildStation.h"
#include "ShelfSpawnZone.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "GameFramework/PlayerController.h"

UShopBuildWidget::UShopBuildWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true; // Q키(닫기) 캡처를 위해 필요
}

void UShopBuildWidget::Setup(ACH6_Shop_SimulatorCharacter* InCharacter, AShopBuildStation* InStation)
{
	OwningCharacter = InCharacter;
	OwningStation = InStation;
}

void UShopBuildWidget::TryBuy(TSubclassOf<ADisplayBase> DisplayClass, int32 Cost)
{
	if (OwningCharacter)
	{
		// 도면에 지정된 스폰 구역 안에서 랜덤한 위치를 뽑음 (없으면 캐릭터 위치로 대체)
		const FVector SpawnLocation = (OwningStation && OwningStation->SpawnZone)
			? OwningStation->SpawnZone->GetRandomSpawnPoint()
			: OwningCharacter->GetActorLocation();

		if (OwningCharacter->BuyAndPlaceDisplay(DisplayClass, Cost, SpawnLocation))
		{
			// 구매 성공하면 UI 닫고 바로 배치(고스트) 모드로 넘어감
			CloseMenu();
		}
		// 실패(골드 부족)하면 BuyAndPlaceDisplay 안에서 이미 화면 메시지를 띄우니
		// 여기서는 UI를 닫지 않고 그대로 둠 (다시 시도하거나 닫기 버튼 누르게)
	}
}

void UShopBuildWidget::OnBuyWeaponClicked()
{
	TryBuy(WeaponShelfClass, WeaponShelfCost);
}

void UShopBuildWidget::OnBuyArmorClicked()
{
	TryBuy(ArmorShelfClass, ArmorShelfCost);
}

void UShopBuildWidget::CloseMenu()
{
	if (OwningCharacter)
	{
		OwningCharacter->bIsInteractionLocked = false;

		if (APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController()))
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}

	RemoveFromParent();
}

void UShopBuildWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsFocusable = true;
	SetKeyboardFocus();
}

FReply UShopBuildWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Q)
	{
		CloseMenu();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}