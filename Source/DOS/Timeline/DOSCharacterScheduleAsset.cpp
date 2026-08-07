#include "DOSCharacterScheduleAsset.h"

namespace
{
	bool IsOverrideApplicable(const FDOSScheduleOverride& Override, const TSet<FName>& ActiveInterventions)
	{
		for (const FName& Required : Override.RequiredInterventions)
		{
			if (!ActiveInterventions.Contains(Required))
			{
				return false;
			}
		}
		return true;
	}
}

void UDOSCharacterScheduleAsset::BuildResolvedWaypoints(const TSet<FName>& ActiveInterventions, TArray<FDOSWaypoint>& OutWaypoints) const
{
	OutWaypoints = BaseWaypoints;
	OutWaypoints.StableSort([](const FDOSWaypoint& A, const FDOSWaypoint& B) { return A.TimeSeconds < B.TimeSeconds; });

	TArray<const FDOSScheduleOverride*> ApplicableOverrides;
	for (const FDOSScheduleOverride& Override : Overrides)
	{
		if (IsOverrideApplicable(Override, ActiveInterventions))
		{
			ApplicableOverrides.Add(&Override);
		}
	}

	// Apply in chronological order: each override truncates anything at/after its own
	// EffectiveFromTimeSeconds (including a prior override's tail) and appends its own
	// waypoints, so a later-starting override always wins from its point onward.
	// TArray<T*>::StableSort auto-dereferences pointer elements before calling the
	// predicate, so this takes references to the pointed-to structs, not the pointers.
	ApplicableOverrides.StableSort([](const FDOSScheduleOverride& A, const FDOSScheduleOverride& B)
	{
		return A.EffectiveFromTimeSeconds < B.EffectiveFromTimeSeconds;
	});

	for (const FDOSScheduleOverride* Override : ApplicableOverrides)
	{
		OutWaypoints.RemoveAll([Override](const FDOSWaypoint& Waypoint)
		{
			return Waypoint.TimeSeconds >= Override->EffectiveFromTimeSeconds;
		});
		OutWaypoints.Append(Override->OverrideWaypoints);
	}

	OutWaypoints.StableSort([](const FDOSWaypoint& A, const FDOSWaypoint& B) { return A.TimeSeconds < B.TimeSeconds; });
}
