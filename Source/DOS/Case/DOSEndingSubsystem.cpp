#include "DOSEndingSubsystem.h"
#include "DOSCaseSubsystem.h"
#include "DOSEndingRuleAsset.h"

void UDOSEndingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedCaseSubsystem = Collection.InitializeDependency<UDOSCaseSubsystem>();
}

FName UDOSEndingSubsystem::EvaluateEnding() const
{
	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	if (!EndingRules || !Case)
	{
		return NAME_None;
	}

	const TSet<FName>& ActiveInterventions = Case->GetActiveInterventions();

	const FDOSEndingRule* BestMatch = nullptr;
	for (const FDOSEndingRule& Rule : EndingRules->Rules)
	{
		if (Rule.Matches(ActiveInterventions) && (!BestMatch || Rule.Priority > BestMatch->Priority))
		{
			BestMatch = &Rule;
		}
	}

	return BestMatch ? BestMatch->EndingId : NAME_None;
}

FName UDOSEndingSubsystem::ResolveEnding()
{
	const FName EndingId = EvaluateEnding();
	if (!EndingId.IsNone())
	{
		OnEndingResolved.Broadcast(EndingId);
	}
	return EndingId;
}
