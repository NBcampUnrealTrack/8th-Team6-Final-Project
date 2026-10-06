// Fill out your copyright notice in the Description page of Project Settings.
//
// 게임 시작 화면. NEW GAME / LOAD GAME / QUIT 세 가지를 제공함.
//
// UMG 구성:
// - 새 게임 버튼의 OnClicked에 StartNewGame() 연결
// - 이어하기 버튼의 OnClicked에 ContinueGame() 연결
// - 종료 버튼의 OnClicked에 QuitGame() 연결
// - 이어하기 버튼을 LoadGameButton이라는 이름으로 만들면, 저장 파일이 없을 때
//   자동으로 비활성화됨 (안 만들어도 동작에는 지장 없음)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

UCLASS()
class CH6_SHOP_SIMULATOR_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 처음부터 새로 시작. 저장 데이터를 무시하고 게임 레벨을 염.
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void StartNewGame();

	// 마지막 저장 지점부터 이어서 시작.
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ContinueGame();

	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void QuitGame();

	// ★ 게임을 시작할 레벨 이름. 블루프린트에서 실제 레벨 이름으로 바꿀 것.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
	FName GameLevelName = TEXT("L_MainLevelProto");

protected:
	virtual void NativeConstruct() override;

	// 저장 파일이 없으면 자동으로 비활성화할 이어하기 버튼.
	// UMG에서 이 이름으로 만들면 연결됨 (없어도 무방).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadGameButton;

private:
	// 마우스 커서를 켜고 UI 입력 모드로 전환
	void SetupMenuInput();

	// 레벨로 넘어가기 전에 게임 입력 모드로 되돌림
	void RestoreGameInput();
};