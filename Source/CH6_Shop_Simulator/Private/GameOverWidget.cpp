// Fill out your copyright notice in the Description page of Project Settings.

#include "GameOverWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameTimeManager.h"

void UGameOverWidget::ShowResult(int32 SurvivedDays)
{
	if (SurvivedDaysText)
	{
		SurvivedDaysText->SetText(FText::FromString(
			FString::Printf(TEXT("Day %d까지 버텼습니다"), SurvivedDays)));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UGameOverWidget::RestartGame()
{
	// ★ 일시정지를 먼저 풀어야 함. 멈춘 상태로 레벨을 다시 열면
	// 새로 시작한 레벨도 멈춘 채로 시작됨.
	UGameplayStatics::SetGamePaused(this, false);

	// 처음부터 다시 하는 것이므로 저장을 불러오지 않음
	AGameTimeManager::bShouldLoadOnStart = false;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	// 현재 레벨을 다시 열어서 처음부터
	const FName CurrentLevel(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevel);
}

void UGameOverWidget::ReturnToMainMenu()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UGameOverWidget::QuitGame()
{
	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}