// 문서 전체 화면. 일기·유언장·각서·편지를 펼쳐 보여준다.
//
// 이 게임은 무언극이라 **텍스트가 정보 전달의 유일한 통로**다. 그래서 토스트로 흘리지 않고
// 화면을 덮어 읽게 한다.
//
// **읽는 동안 타임라인은 멈춘다.** 멈추는 것과 푸는 것은 ADOSPlayerController가 하고,
// 이 위젯은 닫을 때 CloseDocument()를 부르기만 하면 된다 — **부르지 않으면 시간이 멈춘 채로
// 남는다.** NativeDestruct에서도 한 번 더 보장한다.
//
// 표시(폰트·배경·여백)는 블루프린트 몫이다. BP_OnShowDocument로 Title/Body를 받는다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DOSDocumentWidget.generated.h"

class UButton;
class ADOSPlayerController;

UCLASS()
class DOS_API UDOSDocumentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 컨트롤러의 OnDocumentOpened가 이걸 부른다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Document")
	void ShowDocument(const FText& InTitle, const FText& InBody);

	// 닫기 버튼·ESC·바깥 클릭이 전부 이리로 모인다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Document")
	void CloseDocument();

	// 표시는 블루프린트가 한다. 텍스트 블록에 Title/Body를 꽂으면 된다.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Document")
	void BP_OnShowDocument(const FText& InTitle, const FText& InBody);

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Document")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Document")
	FText Body;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// 아무 키나 눌러도 닫힌다. 문서를 읽는 중에는 다른 조작이 없으므로 막을 이유가 없다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Document", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	UFUNCTION()
	void HandleCloseClicked();

	ADOSPlayerController* GetDOSController() const;

	// 두 번 닫히는 것을 막는다(닫기 버튼 + NativeDestruct).
	bool bClosed = false;
};
