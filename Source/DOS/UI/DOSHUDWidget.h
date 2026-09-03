// 상시 화면(HUD). 지금은 **상호작용 키 안내** 하나만 한다.
//
// 이 게임은 무언극이라 조작 안내가 화면과 소리뿐이다. 대상이 잡히면 `[E] 밀기`처럼
// 띄우고, 놓치면 지운다. 안내가 안 뜨는 물건은 **아직 열리지 않은 것**이다 —
// 1회차이거나, 목격하지 않았거나, 시간창을 지난 것. 컨트롤러가 거기서 걸러주므로
// 위젯은 「뜬다/안 뜬다」만 그리면 된다.
//
// 표시(위치·폰트·페이드)는 전부 블루프린트 몫이다. 이 클래스는 컨트롤러의 이벤트를
// 받아 BP 이벤트로 넘기고, Prompt/bHasTarget을 읽을 수 있게 들고만 있는다.
//
// 두드리기 소리는 여기가 아니다 - `DOS Rhythm Intervention` 컴포넌트의 `OnKnock`에
// 물건 쪽 BP에서 직접 건다. 박자마다 소리를 내는 것은 물건의 일이다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Case/DOSInterventionComponent.h"
#include "DOSHUDWidget.generated.h"

UCLASS()
class DOS_API UDOSHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 대상이 바뀌었다. bHasTarget이 거짓이면 안내를 지운다.
	//
	// **매 프레임 오지 않는다** - 컨트롤러가 대상이 실제로 바뀔 때만 쏘므로,
	// 여기서 애니메이션을 재생해도 매 프레임 다시 시작되지 않는다.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|HUD")
	void BP_OnPromptChanged(bool bInHasTarget, const FText& InPrompt);

	// 눌렀을 때의 결과. 소리를 여기에 건다.
	//
	// ⚠ `Acknowledged`에는 **아무 소리도 내지 말 것.** 리듬 노크가 진행 중이라는 뜻이고,
	// 두드리는 소리는 `OnKnock`이 낸다. 여기서 또 내면 두 번 겹친다.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|HUD")
	void BP_OnInteractResult(EDOSInteractResult Result, FName InterventionId);

	// 지금 안내가 떠 있는지. BP에서 Visibility를 바인딩할 때 쓴다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|HUD")
	bool bHasTarget = false;

	// 지금 대상의 안내 문구("밀기" / "두드리기" / "읽기"). 키 이름은 BP에서 붙인다 -
	// 나중에 키 리바인딩을 넣으면 여기가 아니라 표시 쪽에서 바뀌어야 한다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|HUD")
	FText Prompt;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePromptChanged(bool bInHasTarget, const FText& InPrompt);

	UFUNCTION()
	void HandleInteractResult(EDOSInteractResult Result, FName InterventionId);
};
