#include "DOSTimelineLibrary.h"

FGameplayTag UDOSTimelineLibrary::MakeActionTag(FName TagName)
{
	if (TagName.IsNone())
	{
		return FGameplayTag();
	}

	// ErrorIfNotFound=false: 없는 이름이면 조용히 빈 태그를 돌려준다. 그 대신 아래에서
	// 직접 경고를 남긴다 - 엔진 기본 에러는 어느 인물의 어느 박자인지 알려주지 않는다.
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] 등록되지 않은 GameplayTag입니다: %s - Config/DefaultGameplayTags.ini를 확인하세요. ")
			TEXT("빈 태그가 들어가면 Action.Dead 판정이 조용히 실패합니다."),
			*TagName.ToString());
	}
	return Tag;
}

FName UDOSTimelineLibrary::GetActionTagName(FGameplayTag Tag)
{
	return Tag.IsValid() ? Tag.GetTagName() : NAME_None;
}
