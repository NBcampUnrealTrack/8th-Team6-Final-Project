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
