// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuWidget.h"
#include "GameTimeManager.h"
#include "ShopSaveGame.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetupMenuInput();

	// ★ 저장 파일이 없으면 이어하기를 못 누르게 막음.
	// (눌러도 아무 일도 안 일어나면 플레이어가 혼란스러우므로)
	if (LoadGameButton)
	{
		LoadGameButton->SetIsEnabled(UGameplayStatics::DoesSaveGameExist(UShopSaveGame::SlotName, 0));
	}
}

void UMainMenuWidget::SetupMenuInput()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

void UMainMenuWidget::StartNewGame()
{
	// ★ 이어하기 플래그를 꺼둬야 함. 이전에 LOAD로 들어갔다가 메뉴로
	// 돌아온 경우, 플래그가 남아있으면 새 게임인데 저장을 불러와버림.
	AGameTimeManager::bShouldLoadOnStart = false;

	// 새로 시작하는 것이므로 기존 저장은 지움
	// (다음 하루가 시작될 때 새 데이터로 다시 저장됨)
	if (UGameplayStatics::DoesSaveGameExist(UShopSaveGame::SlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(UShopSaveGame::SlotName, 0);
	}

	RestoreGameInput();
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UMainMenuWidget::ContinueGame()
{
	if (!UGameplayStatics::DoesSaveGameExist(UShopSaveGame::SlotName, 0))
	{
		return; // 저장이 없으면 아무 일도 안 함
	}

	// ★ 레벨을 열기 전에 플래그를 켜둠. 새 레벨의 GameTimeManager가
	// BeginPlay에서 이 값을 보고 저장 데이터를 불러옴.
	AGameTimeManager::bShouldLoadOnStart = true;

	RestoreGameInput();
	UGameplayStatics::OpenLevel(this, GameLevelName);
}

void UMainMenuWidget::RestoreGameInput()
{
	// ★ 메뉴에서 UIOnly로 바꿔둔 입력 모드를 게임용으로 되돌림.
	// 이걸 안 하면 레벨로 넘어가도 조작이 안 되고 커서만 떠있음.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	// 메뉴 위젯도 화면에서 제거
	RemoveFromParent();
}

void UMainMenuWidget::QuitGame()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}