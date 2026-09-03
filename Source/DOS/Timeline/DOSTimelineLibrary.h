// 에디터 스크립트(파이썬)에서 부르기 위한 헬퍼.
//
// **왜 필요한가**: 파이썬에서 FGameplayTag를 이름으로 만들 확실한 방법이 없다.
//   - `unreal.GameplayTag(tag_name=...)`  → 그런 생성자 인자가 없다
//   - `FGameplayTag::RequestGameplayTag`  → C++ 전용. 블루프린트에 노출되지 않았다
//   - `TagName` 프로퍼티                  → VisibleAnywhere(읽기 전용)
//   - `MakeLiteralGameplayTag`            → 이미 태그를 받으므로 순환
//
// BlueprintFunctionLibrary는 항상 파이썬에 노출되므로 여기를 통하면 확실하다.
//   unreal.DOSTimelineLibrary.make_action_tag("Action.Idle")

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "DOSTimelineLibrary.generated.h"

UCLASS()
class DOS_API UDOSTimelineLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 이름으로 GameplayTag를 만든다. 등록되지 않은 이름이면 경고를 남기고 빈 태그를
	// 돌려준다 - 조용히 빈 태그가 들어가면 나중에 왜 죽지 않는지(Action.Dead) 찾기 어렵다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	static FGameplayTag MakeActionTag(FName TagName);

	// 위의 반대쪽. 태그에서 이름을 되꺼낸다. 빈 태그면 NAME_None.
	//
	// **왜 필요한가**: 파이썬에서 `unreal.GameplayTag`는 프로퍼티가 하나도 노출되지 않는다
	// (repr이 `{}`로 나온다). TagName이 VisibleAnywhere여도 파이썬에서는 못 읽는다.
	// 일정표를 CSV로 뽑을 때 이게 없으면 ActionTag 칸이 전부 빈 채로 나가고, 다시
	// 넣는 순간 모든 인물의 동작 태그가 조용히 지워진다.
	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	static FName GetActionTagName(FGameplayTag Tag);
};
