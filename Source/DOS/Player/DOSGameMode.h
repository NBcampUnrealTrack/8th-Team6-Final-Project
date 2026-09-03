// 게임플레이용 GameMode. 기존 DOSMainMenuGameMode는 메뉴 레벨(L_MainMenu) 전용이고,
// 이쪽은 저택 안에서 30분 타임라인이 도는 레벨에 붙는다.
//
// 하는 일은 둘뿐이다: 관찰자 폰/컨트롤러를 기본값으로 물리고, 레벨이 열리면 타임라인을
// 돌리기 시작한다. 나머지는 전부 서브시스템(Timeline/Case/Ending)이 알아서 한다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DOSGameMode.generated.h"

UCLASS()
class DOS_API ADOSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADOSGameMode();

	// 레벨이 열리자마자 타임라인을 돌릴지. 블록아웃 단계에서 정지 상태로 둘러보고 싶을 때
	// 끄면 된다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Timeline")
	bool bAutoPlayTimeline = true;

	// 관찰용 배속. 1회차는 빨리감기로 훑는 구조이므로 테스트할 때도 올려 쓰는 일이 많다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Timeline", meta = (ClampMin = "0.1"))
	float InitialPlaybackRate = 1.f;

protected:
	virtual void BeginPlay() override;
};
