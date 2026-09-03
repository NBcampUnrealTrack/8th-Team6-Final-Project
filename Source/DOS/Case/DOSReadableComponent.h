// 읽기 전용 문서 - 일기, 유언장, 편지 같은 것. 밀 수 없으므로 개입(UDOSInterventionComponent)과
// 별개다. 무언극이라 대사가 없고, 텍스트는 이 문서들로만 들어온다.
//
// **텍스트는 반드시 FText다.** FString이면 번역 대상에서 빠진다 - 이 게임은 5개 국어
// (en/ja/zh-Hans/zh-Hant/ko)로 나가므로 화면에 뜨는 모든 글자가 FText여야 한다.
//
// ⚠ **본문을 파이썬으로 임포트하지 말 것.** 파이썬이 만드는 FText는 키가 없는
// culture-invariant 텍스트라 Localization Dashboard가 수집하지 못한다. 다시 임포트하면
// 키가 바뀌어 기존 번역이 통째로 떨어져 나가기도 한다. **에디터에서 String Table을 참조하거나
// 직접 입력한다.** 스케줄 임포터가 텍스트를 하나도 안 넣는 것도 같은 이유다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DOSReadableComponent.generated.h"

class UDOSTimelineSubsystem;
class UDOSCaseSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDOSOnDocumentRead);

UCLASS(ClassGroup = (DOS), meta = (BlueprintSpawnableComponent))
class DOS_API UDOSReadableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDOSReadableComponent();

	// 읽음 기록과 도전과제 키. UDOSCaseSubsystem의 CollectedClues에 이 이름으로 남는다.
	// 예: doc_rose_diary / doc_will / doc_doctor_letter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	FName DocumentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document", meta = (MultiLine = "true"))
	FText Body;

	// 화면에 뜨는 동사.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	FText PromptText;

	// 이 구간에만 읽힌다. 유언장은 빅토리아가 펼쳐두는 10:50~15:00에만 읽을 수 있고,
	// 일기는 감옥에 늘 있으므로 구간 전체다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	float WindowStartSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	float WindowEndSeconds = 1800.f;

	// 읽으면 얻는 지식. 로즈의 일기가 `witness_rose_name`을 준다 - 그래야 엔딩에서
	// `???`가 이름으로 바뀐다. 비워두면 읽기만 하고 아무것도 열지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Document")
	FName GrantsWitnessId = NAME_None;

	// 소리·연출을 여기에 건다.
	UPROPERTY(BlueprintAssignable, Category = "DOS|Document")
	FDOSOnDocumentRead OnDocumentRead;

	UFUNCTION(BlueprintPure, Category = "DOS|Document")
	bool IsWithinWindow() const;

	// 이번 세션에 이미 읽었는가. 회차가 바뀌어도 남는다 - 읽은 글을 잊을 이유가 없다.
	UFUNCTION(BlueprintPure, Category = "DOS|Document")
	bool HasBeenRead() const;

	// 읽음으로 기록하고 지식을 준다. 컨트롤러가 문서 화면을 열 때 부른다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Document")
	bool MarkRead();

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
};
