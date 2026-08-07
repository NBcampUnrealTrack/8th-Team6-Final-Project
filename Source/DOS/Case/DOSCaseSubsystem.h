// Tracks the current case's branching state: which interventions are active THIS round
// (reset every time the timeline restarts a new round) and which clues have ever been
// discovered THIS session (never reset - clues do not affect gameplay, they only gate
// one-shot notification/achievement hooks, see DiscoverClue).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DOSCaseSubsystem.generated.h"

class UDOSTimelineSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDOSOnActiveInterventionsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnClueDiscovered, FName, ClueId);

UCLASS()
class DOS_API UDOSCaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Adds InterventionId to this round's active set. Returns false if it was already active
	// this round (e.g. re-triggering the same object) or InterventionId is none.
	UFUNCTION(BlueprintCallable, Category = "DOS|Case")
	bool RegisterIntervention(FName InterventionId);

	// Marks ClueId as discovered. Returns true only the very first time this session, so
	// callers can gate one-shot notification UI / achievement unlocks; false on every repeat.
	UFUNCTION(BlueprintCallable, Category = "DOS|Case")
	bool DiscoverClue(FName ClueId);

	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	const TSet<FName>& GetActiveInterventions() const { return ActiveInterventions; }

	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	bool HasIntervened(FName InterventionId) const { return ActiveInterventions.Contains(InterventionId); }

	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	bool HasDiscoveredClue(FName ClueId) const { return CollectedClues.Contains(ClueId); }

	// Fires whenever ActiveInterventions changes (new intervention, or reset on round
	// restart). NPC schedule components should refresh off this - see
	// UDOSCharacterScheduleComponent::RefreshResolvedSchedule.
	UPROPERTY(BlueprintAssignable, Category = "DOS|Case")
	FDOSOnActiveInterventionsChanged OnActiveInterventionsChanged;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Case")
	FDOSOnClueDiscovered OnClueDiscovered;

private:
	UFUNCTION()
	void HandleRoundRestarted(int32 NewRound);

	UPROPERTY(Transient)
	TSet<FName> ActiveInterventions;

	UPROPERTY(Transient)
	TSet<FName> CollectedClues;

	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
};
