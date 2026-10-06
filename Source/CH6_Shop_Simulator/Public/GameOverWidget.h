// Fill out your copyright notice in the Description page of Project Settings.
//
// 코어가 파괴되어 게임이 끝났을 때 뜨는 화면.
// 몇 일차까지 버텼는지 보여주고, 다시 시작하거나 종료할 수 있게 함.
//
// UMG 구성:
// - SurvivedDaysText: "Day 7까지 버텼습니다" 같은 결과 문구
// - 다시 시작 버튼의 OnClicked에 RestartGame() 연결
// - 종료 버튼의 OnClicked에 QuitGame() 연결

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UTextBlock;

UCLASS()
class CH6_SHOP_SIMULATOR_API UGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 게임 오버 화면을 띄울 때 호출. 버틴 날짜를 받아 문구를 채움.
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void ShowResult(int32 SurvivedDays);

	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void RestartGame();

	// 메인 메뉴로 돌아가기
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void ReturnToMainMenu();

	// 메인 메뉴 레벨 이름. 블루프린트에서 실제 이름으로 바꿀 것.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Over")
	FName MainMenuLevelName = TEXT("L_MainMenu");

	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void QuitGame();

protected:
	// 결과 문구를 표시할 텍스트. UMG에서 이 이름으로 만들 것.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SurvivedDaysText;
};