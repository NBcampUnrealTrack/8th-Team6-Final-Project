// 레벨 경로를 한 군데로 모은 것. Project Settings -> Game -> DOS Levels 에서 지정한다.
//
// 종전에는 두 파일에 **하드코딩**되어 있었다 —
//   DOSSaveGameSubsystem.cpp  GameplayLevelPath = "/Game/DoS/Maps/L_TestRoom"  (8월에 만든 테스트 방)
//   DOSPauseWidget.cpp        MainMenuLevelPath = "/Game/DoS/Maps/L_MainMenu"   (아직 없는 레벨)
// 레벨 이름을 바꿀 때마다 C++을 고쳐야 했고, 실제로 새 게임이 낡은 테스트 방을 열고 있었다.
//
// TSoftObjectPtr<UWorld>라 에디터에서 **레벨을 골라서** 넣는다. 문자열을 타이핑하지 않으므로
// 오타로 조용히 실패하지 않고, 레벨을 옮기면 참조가 따라간다.
//
// 왜 게임모드가 아니라 여기인가: 메인메뉴와 게임플레이가 **서로 다른 게임모드**를 쓰는데
// 양쪽 다 두 경로를 알아야 한다(메뉴는 게임플레이 레벨로 가고, 일시정지는 메뉴로 돌아온다).
// UDeveloperSettings만이 Project Settings에 제 페이지를 갖는다 - UDOSAudioSettings와 같은 이유다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DOSLevelSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "DOS Levels"))
class DOS_API UDOSLevelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	// 메인메뉴 · 프롤로그 · 커튼콜이 도는 레벨. 저택 외관과 기도실만 있는 작은 세트를
	// 상정한다 - 프롤로그가 그 무대를 그대로 쓰고, 연출이 도는 동안 게임플레이 레벨을
	// 로드할 수 있다.
	UPROPERTY(EditAnywhere, Config, Category = "DOS|Levels")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	// 30분 회차가 도는 저택.
	UPROPERTY(EditAnywhere, Config, Category = "DOS|Levels")
	TSoftObjectPtr<UWorld> GameplayLevel;

	// 비어 있으면 로그를 남기고 아무 일도 하지 않는다 - 잘못된 레벨을 여는 것보다는
	// 안 여는 편이 원인을 찾기 쉽다.
	static const UDOSLevelSettings* Get() { return GetDefault<UDOSLevelSettings>(); }
};
