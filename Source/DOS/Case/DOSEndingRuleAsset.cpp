#include "DOSEndingRuleAsset.h"

bool FDOSEndingRule::Matches(const TSet<FName>& ActiveInterventions) const
{
	for (const FName& Required : RequiredInterventions)
	{
		if (!ActiveInterventions.Contains(Required))
		{
			return false;
		}
	}

	for (const FName& Forbidden : ForbiddenInterventions)
	{
		if (ActiveInterventions.Contains(Forbidden))
		{
			return false;
		}
	}

	return true;
}
