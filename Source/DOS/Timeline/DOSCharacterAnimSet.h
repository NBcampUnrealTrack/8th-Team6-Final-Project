// 「이 인물은 이 행동을 어떻게 하는가」를 담는 표.
//
// **ActionTag만으로는 부족하다.** 같은 `Action.Working`이라도 앨리스의 설거지와 다른 인물의
// 일거리가 같은 동작일 이유가 없다. 태그 하나에 동작 하나를 묶으면 일곱 명이 똑같이 움직인다.
//
// 그래서 세 단으로 찾는다 (ADOSScheduledCharacter::ResolveActionMontage):
//
//   1. 웨이포인트의 ActionMontage    그 박자만의 것        빅토리아의 왈츠, 로즈의 목 조르기
//   2. 인물별 AnimSet                그 인물다운 기본       앨리스의 "일하기"
//   3. 공용 SharedAnimSet            아무 인물이나          그냥 서 있기, 걷기
//
// 인물별 세트에는 **공용과 달라야 하는 것만** 넣는다. 나머지는 3단으로 떨어진다.
//
// 왜 자식 BP의 TMap이 아니라 DataAsset인가: 블루프린트 자식 클래스는 **맵의 개별 항목을
// 덮어쓸 수 없다** - 맵 전체를 다시 채워야 해서 공용 폴백이 무의미해진다. 그리고 BP를 열지
// 않고 고칠 수 있고 diff도 된다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DOSCharacterAnimSet.generated.h"

class UAnimMontage;

UCLASS(BlueprintType)
class DOS_API UDOSCharacterAnimSet : public UDataAsset
{
	GENERATED_BODY()

public:
	// 누구의 것인지 적어둔다(alice / victoria / ...). 공용 세트는 비워둔다.
	// 실제 선택은 액터에 무엇을 꽂았는지로 결정되고, 이 값은 표시·검증용이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|AnimSet")
	FName CharacterId = NAME_None;

	// ActionTag -> 몽타주. **정확히 일치하는 태그만** 본다(계층 폴백 없음).
	// 지금 태그가 전부 평면(Action.Idle, Action.Working ...)이므로 그걸로 충분하다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|AnimSet")
	TMap<FGameplayTag, TSoftObjectPtr<UAnimMontage>> ByActionTag;

	// 없으면 nullptr. 있으면 동기 로드해서 돌려준다 - 몽타주는 작고, 박자가 바뀔 때만
	// 부르므로 매 프레임 비용이 아니다.
	UFUNCTION(BlueprintCallable, Category = "DOS|AnimSet")
	UAnimMontage* Find(FGameplayTag ActionTag) const;
};
