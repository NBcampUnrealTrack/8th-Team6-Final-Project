// Drives a single NPC actor's transform/action from a UDOSCharacterScheduleAsset, sampled
// against the master timeline. Depends only on UDOSTimelineSubsystem (System 1) - it knows
// nothing about interventions/case state as a class dependency, only as opaque FName data
// passed into RefreshResolvedSchedule(). Wiring "case state changed -> refresh this
// component" is left to the owning NPC actor/Blueprint so System 1 stays independently
// testable without System 2 present.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DOSTimelineTypes.h"
#include "DOSCharacterScheduleComponent.generated.h"

class UDOSCharacterScheduleAsset;
class UDOSTimelineSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnCharacterStateApplied, const FDOSCharacterState&, State);

UCLASS(ClassGroup = (DOS), meta = (BlueprintSpawnableComponent))
class DOS_API UDOSCharacterScheduleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDOSCharacterScheduleComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Schedule")
	TObjectPtr<UDOSCharacterScheduleAsset> ScheduleAsset;

	// 샘플링한 위치를 소유 액터의 트랜스폼에 곧바로 찍을지. 기본값은 true(스크립트대로 순간
	// 배치)지만, **NavMesh로 실제로 걷게 하려면 꺼야 한다** - 켜 두면 매 틱 순간이동이
	// 걸음을 덮어써서 이동이 성립하지 않는다. 끄면 OnCharacterStateApplied만 쏘고 트랜스폼은
	// 건드리지 않으므로, 소유 액터가 그 목표 지점까지 알아서 걸어가면 된다.
	// (ADOSScheduledCharacter가 이 방식으로 쓴다)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DOS|Schedule")
	bool bApplyTransformToOwner = true;

	// Recomputes the resolved (base + applicable overrides) waypoint list. Call this whenever
	// the current round's active interventions change - the owning actor/Blueprint is
	// responsible for listening to UDOSCaseSubsystem::OnActiveInterventionsChanged and
	// forwarding it here.
	UFUNCTION(BlueprintCallable, Category = "DOS|Schedule")
	void RefreshResolvedSchedule(const TSet<FName>& ActiveInterventions);

	// Pure query: what should this character's state be at time T given the currently
	// resolved schedule? Safe to call repeatedly for scrubbing/rewinding.
	UFUNCTION(BlueprintCallable, Category = "DOS|Schedule")
	FDOSCharacterState SampleAtTime(float TimeSeconds) const;

	// Broadcast every time the timeline moves and a new state is applied to the owning actor.
	// Blueprint hooks animation montage playback / interaction-object visuals off this.
	UPROPERTY(BlueprintAssignable, Category = "DOS|Schedule")
	FDOSOnCharacterStateApplied OnCharacterStateApplied;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTimelineTimeChanged(float NewTimeSeconds);

	void ApplyState(const FDOSCharacterState& State);

	UPROPERTY(Transient)
	TArray<FDOSWaypoint> ResolvedWaypoints;

	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
};
