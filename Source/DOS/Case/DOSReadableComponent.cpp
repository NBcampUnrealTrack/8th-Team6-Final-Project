#include "DOSReadableComponent.h"

#include "DOSCaseSubsystem.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "DOSReadable"

UDOSReadableComponent::UDOSReadableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PromptText = LOCTEXT("DefaultReadPrompt", "내용 보기");
}

void UDOSReadableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			CachedTimelineSubsystem = GameInstance->GetSubsystem<UDOSTimelineSubsystem>();
			CachedCaseSubsystem = GameInstance->GetSubsystem<UDOSCaseSubsystem>();
		}
	}

	if (DocumentId.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: Document Id가 비어 있습니다 - 읽어도 기록되지 않고 도전과제도 안 걸립니다."),
			*GetOwner()->GetName());
	}
	if (Body.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: 본문이 비어 있습니다."), *GetOwner()->GetName());
	}
}

bool UDOSReadableComponent::IsWithinWindow() const
{
	const UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get();
	if (!Timeline)
	{
		return false;
	}

	const float Now = Timeline->GetCurrentTime();
	return Now >= WindowStartSeconds && Now <= WindowEndSeconds;
}

bool UDOSReadableComponent::HasBeenRead() const
{
	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	return Case && !DocumentId.IsNone() && Case->HasDiscoveredClue(DocumentId);
}

bool UDOSReadableComponent::MarkRead()
{
	UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	if (!Case || DocumentId.IsNone())
	{
		return false;
	}

	// 처음 읽은 것만 true. 도전과제 알림이 두 번 뜨지 않게 하는 것이 이 반환값의 용도다.
	const bool bFirstTime = Case->DiscoverClue(DocumentId);

	// 읽어서 얻는 지식은 별도 키다. 문서를 읽었다는 사실(도전과제)과, 그 결과로 무엇을
	// 알게 되었는가(게임플레이 잠금 해제)는 서로 다른 것이므로 섞지 않는다.
	if (!GrantsWitnessId.IsNone())
	{
		Case->DiscoverClue(GrantsWitnessId);
	}

	if (bFirstTime)
	{
		UE_LOG(LogTemp, Log, TEXT("[DOS] 문서를 읽었습니다: %s"), *DocumentId.ToString());
		OnDocumentRead.Broadcast();
	}
	return bFirstTime;
}

#undef LOCTEXT_NAMESPACE
