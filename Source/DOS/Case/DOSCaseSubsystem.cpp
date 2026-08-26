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
	OnActiveInterventionsChanged.Broadcast();
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

	CollectedClues.Reset();
	CollectedClues.Append(InCollectedClues);

	OnActiveInterventionsChanged.Broadcast();
}

void UDOSCaseSubsystem::HandleRoundRestarted(int32 NewRound)
{
	if (ActiveInterventions.Num() > 0)
	{
		ActiveInterventions.Reset();
		OnActiveInterventionsChanged.Broadcast();
	}
}
