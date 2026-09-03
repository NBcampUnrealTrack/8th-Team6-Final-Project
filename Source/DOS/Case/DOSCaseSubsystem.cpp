#include "DOSCaseSubsystem.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/GameInstance.h"

void UDOSCaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedTimelineSubsystem = Collection.InitializeDependency<UDOSTimelineSubsystem>();
	if (UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get())
	{
		Timeline->OnRoundRestarted.AddDynamic(this, &UDOSCaseSubsystem::HandleRoundRestarted);
	}
}

bool UDOSCaseSubsystem::RegisterIntervention(FName InterventionId)
{
	if (InterventionId.IsNone() || ActiveInterventions.Contains(InterventionId))
	{
		return false;
	}

	ActiveInterventions.Add(InterventionId);
	// 언제 성립했는지를 같이 남긴다 - 되감기가 이 값 위에 선다.
	const UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get();
	InterventionTimes.Add(InterventionId, Timeline ? Timeline->GetCurrentTime() : 0.f);

	OnActiveInterventionsChanged.Broadcast();
	return true;
}

float UDOSCaseSubsystem::GetInterventionTime(FName InterventionId) const
{
	const float* Found = InterventionTimes.Find(InterventionId);
	return Found ? *Found : -1.f;
}

float UDOSCaseSubsystem::GetLastInterventionTime() const
{
	float Latest = -1.f;
	for (const TPair<FName, float>& Pair : InterventionTimes)
	{
		Latest = FMath::Max(Latest, Pair.Value);
	}
	return Latest;
}

void UDOSCaseSubsystem::RewindTo(float TimeSeconds)
{
	TArray<FName> ToRemove;
	for (const TPair<FName, float>& Pair : InterventionTimes)
	{
		if (Pair.Value >= TimeSeconds)
		{
			ToRemove.Add(Pair.Key);
		}
	}
	if (ToRemove.Num() == 0)
	{
		return;
	}

	for (const FName& Id : ToRemove)
	{
		ActiveInterventions.Remove(Id);
		InterventionTimes.Remove(Id);
		UE_LOG(LogTemp, Log, TEXT("[DOS] 되감기로 개입을 되돌립니다: %s"), *Id.ToString());
	}
	// 단서(CollectedClues)는 건드리지 않는다 - 되감아도 본 것을 잊지는 않는다.
	OnActiveInterventionsChanged.Broadcast();
}

bool UDOSCaseSubsystem::RewindToLastIntervention(float MarginSeconds)
{
	const float Last = GetLastInterventionTime();
	if (Last < 0.f)
	{
		return false; // 되돌릴 개입이 없다.
	}

	const float Target = FMath::Max(0.f, Last - FMath::Max(0.f, MarginSeconds));

	// **상태를 먼저 되돌리고 시간을 옮긴다.** 순서가 반대면 스케줄이 옛 개입 상태로 한 번
	// 평가된 뒤에 갱신되어, 되감은 자리에서 아직 밀지도 않은 결과가 한 프레임 보인다.
	RewindTo(Target);

	if (UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get())
	{
		Timeline->ScrubTo(Target);
	}
	return true;
}

bool UDOSCaseSubsystem::DiscoverClue(FName ClueId)
{
	if (ClueId.IsNone() || CollectedClues.Contains(ClueId))
	{
		return false;
	}

	CollectedClues.Add(ClueId);
	OnClueDiscovered.Broadcast(ClueId);
	return true;
}

void UDOSCaseSubsystem::RestoreState(const TArray<FName>& InActiveInterventions, const TArray<FName>& InCollectedClues)
{
	ActiveInterventions.Reset();
	ActiveInterventions.Append(InActiveInterventions);
	// 세이브에는 시각이 없다. 이어하기 직후에는 "마지막 개입 직전으로"가 그 회차의
	// 시작점으로 잡히는데, 잘못된 자리로 보내는 것보다는 낫다.
	InterventionTimes.Reset();
	for (const FName& Id : InActiveInterventions)
	{
		InterventionTimes.Add(Id, 0.f);
	}

	CollectedClues.Reset();
	CollectedClues.Append(InCollectedClues);

	OnActiveInterventionsChanged.Broadcast();
}

void UDOSCaseSubsystem::HandleRoundRestarted(int32 NewRound)
{
	if (ActiveInterventions.Num() > 0)
	{
		ActiveInterventions.Reset();
		InterventionTimes.Reset();
		OnActiveInterventionsChanged.Broadcast();
	}
}
