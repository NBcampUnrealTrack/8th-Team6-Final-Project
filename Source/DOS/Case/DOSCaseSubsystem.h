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

	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	const TSet<FName>& GetCollectedClues() const { return CollectedClues; }

	// --- 되감기 -------------------------------------------------------------
	//
	// **개입이 언제 성립했는지를 기록한다.** 참/거짓만 저장하면 "마지막 상호작용 직전으로
	// 돌리기"를 계산할 수 없고, 되감아도 밀어놓은 물건이 제자리로 안 돌아간다
	// (20:30에 설탕통을 엎고 19:00으로 되감으면 아직 엎지도 않았는데 엎어져 있다).

	// 마지막으로 성립한 개입의 시각. 하나도 없으면 -1.
	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	float GetLastInterventionTime() const;

	UFUNCTION(BlueprintPure, Category = "DOS|Case")
	float GetInterventionTime(FName InterventionId) const;

	// 그 시각 이후에 성립한 개입을 전부 되돌린다. 되감기가 개입을 그대로 두면 아직 밀지도
	// 않은 시각에 이미 밀린 결과가 재생되므로, 시간과 상태를 같이 되돌려야 한다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Case")
	void RewindTo(float TimeSeconds);

	// 마지막 개입 직전으로 시간과 상태를 함께 되돌린다. 일시정지 화면이 부른다.
	// 되돌릴 개입이 없으면 false.
	UFUNCTION(BlueprintCallable, Category = "DOS|Case")
	bool RewindToLastIntervention(float MarginSeconds = 5.f);

	// Directly sets both sets from save data (e.g. Continue) and broadcasts
	// OnActiveInterventionsChanged so NPC schedules refresh - does not touch CollectedClues
	// listeners since clue rediscovery notifications don't need to re-fire on load.
	UFUNCTION(BlueprintCallable, Category = "DOS|Case")
	void RestoreState(const TArray<FName>& InActiveInterventions, const TArray<FName>& InCollectedClues);

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

	// 개입 id -> 성립한 타임라인 시각.
	UPROPERTY(Transient)
	TMap<FName, float> InterventionTimes;

	UPROPERTY(Transient)
	TSet<FName> CollectedClues;

	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
};
