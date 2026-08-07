// Data-only definition of a single character's schedule for the case. Designed to be filled
// from an external waypoint/schedule editor (e.g. an HTML/Three.js prototype) and imported as
// JSON, so it holds no references to a specific level/actor.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DOSTimelineTypes.h"
#include "DOSCharacterScheduleAsset.generated.h"

// A conditional replacement of part of a character's schedule. Applies only when every
// RequiredIntervention in this override is present in the current round's active
// interventions - this is where System 2 (branching) reaches into System 1 (schedule).
USTRUCT(BlueprintType)
struct FDOSScheduleOverride
{
	GENERATED_BODY()

	// All of these must be active this round for this override to apply.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Schedule")
	TArray<FName> RequiredInterventions;

	// From this time onward, OverrideWaypoints replace the base/previous waypoints, until a
	// later-applying override (if any) takes over.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Schedule")
	float EffectiveFromTimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Schedule")
	TArray<FDOSWaypoint> OverrideWaypoints;
};

UCLASS(BlueprintType)
class DOS_API UDOSCharacterScheduleAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Schedule")
	FName CharacterId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Schedule")
	TArray<FDOSWaypoint> BaseWaypoints;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Schedule")
	TArray<FDOSScheduleOverride> Overrides;

	// Merges BaseWaypoints with every Override whose RequiredInterventions are all satisfied
	// by ActiveInterventions, into a single time-sorted array ready for sampling. Pure
	// function of (BaseWaypoints, Overrides, ActiveInterventions) - no hidden state.
	void BuildResolvedWaypoints(const TSet<FName>& ActiveInterventions, TArray<FDOSWaypoint>& OutWaypoints) const;
};
