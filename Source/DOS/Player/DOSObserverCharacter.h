// 플레이어 캐릭터 - 그레이스 애쉬워스의 유령. 저택을 걸어 다니며 관찰하는 것이 전부다.
//
// 유령 규칙(Docs/Story_Design.md):
//   - 벽을 뚫고 다닐 수 없다 -> 평범한 캡슐 충돌을 그대로 쓴다
//   - 문을 열고 닫을 수 없다 -> 문은 별도 액터가 막는다 (이 클래스의 책임이 아니다)
//   - 오브젝트는 미는 것만 가능 -> 상호작용은 추후 별도 컴포넌트
// 뛰기/점프/웅크리기는 넣지 않는다. 걷는 것과 보는 것뿐이다.
//
// 1인칭이다. 그녀는 아무에게도 보이지 않으므로 몸을 화면에 둘 이유가 없고, 열쇠 구멍으로
// 들여다보는 연출과 "곁에 서야 성립하는" 원온원이 전부 1인칭에 맞는다.
// 3인칭으로 바꾸려면 SpringArm을 하나 끼우고 카메라를 그 끝에 붙이면 된다.
//
// **입력은 이 클래스의 책임이 아니다.** 매핑 컨텍스트 등록과 액션 바인딩은 전부
// ADOSPlayerController가 한다. 여기서도 같이 하면 입력이 두 번 들어가는데, 이동은
// CharacterMovement가 입력 벡터를 정규화해 두 배가 지워지지만 회전은 그냥 누적되어
// 시점이 두 배로 돈다 - 원인을 찾기 아주 어려운 증상이 된다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DOSObserverCharacter.generated.h"

class UCameraComponent;

UCLASS()
class DOS_API ADOSObserverCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADOSObserverCharacter();

	UCameraComponent* GetObserverCamera() const { return Camera; }

private:
	UPROPERTY(VisibleAnywhere, Category = "DOS|Camera")
	TObjectPtr<UCameraComponent> Camera;
};
