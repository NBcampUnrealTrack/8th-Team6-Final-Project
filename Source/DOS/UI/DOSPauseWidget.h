// 일시정지 화면.
//
//   계속하기  /  마지막 상호작용 직전으로  /  처음부터  /  설정  /  메인메뉴로
//
// ⚠ **「처음부터」는 RestartRound()다. RestartLoop()이 아니다.**
// 그쪽은 회차 번호를 올리므로, 1회차에서 「처음부터」를 누르면 관찰도 안 한 채 2회차가 되어
// 개입 권한이 열려버린다.
//
// ⚠ **되감기는 자유 스크럽이 아니다.** 시간을 마음대로 끌 수 있으면 노크의 15초 창을 외워서
// 치는 설계가 통째로 무너진다. 「마지막 개입 직전」이라는 체크포인트로만 돌아가고,
// 그 개입 자체도 같이 취소된다(단서는 남는다).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DOSPauseWidget.generated.h"

class UButton;
class UDOSSettingsWidget;

UCLASS()
class DOS_API UDOSPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 열 때 부른다. 게임을 멈추고 입력 모드를 UI로 돌린다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Pause")
	void OpenPause();

	UFUNCTION(BlueprintCallable, Category = "DOS|Pause")
	void ClosePause();

	// 되돌릴 개입이 없으면 「마지막 상호작용 직전으로」는 눌러도 아무 일이 없다.
	// 버튼을 흐리게 하거나 숨기는 것은 블루프린트가 한다.
	UFUNCTION(BlueprintPure, Category = "DOS|Pause")
	bool CanRewindToLastIntervention() const;

	// 버튼 상태를 갱신하라는 신호. OpenPause에서 한 번 쏜다.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Pause")
	void BP_OnRefreshState(bool bCanRewind);

	// 설정 화면을 띄운다. 메인메뉴의 것과 같은 위젯을 쓴다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Pause")
	TSubclassOf<UDOSSettingsWidget> SettingsWidgetClass;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Pause", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Pause", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RewindButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Pause", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartRoundButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Pause", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Pause", meta = (BindWidgetOptional))
	TObjectPtr<UButton> MainMenuButton;

private:
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleRewind();
	UFUNCTION() void HandleRestartRound();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleMainMenu();
};
