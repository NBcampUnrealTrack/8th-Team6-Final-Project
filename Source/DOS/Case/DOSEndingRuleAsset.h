// Data-driven ending rules. A rule matches the current round's active interventions; the
// concrete intervention IDs and combinations are design data, filled in later without any
// code change.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DOSEndingRuleAsset.generated.h"

USTRUCT(BlueprintType)
struct FDOSEndingRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Ending")
	FName EndingId = NAME_None;

	// When multiple rules match, the highest Priority wins.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Ending")
	int32 Priority = 0;

	// All of these must be present in the current round's active interventions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Ending")
	TArray<FName> RequiredInterventions;

	// None of these may be present in the current round's active interventions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Ending")
	TArray<FName> ForbiddenInterventions;

	bool Matches(const TSet<FName>& ActiveInterventions) const;
};

UCLASS(BlueprintType)
class DOS_API UDOSEndingRuleAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Designers should include a catch-all rule (empty Required/Forbidden, lowest Priority)
	// so EvaluateEnding() always has a fallback match.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Ending")
	TArray<FDOSEndingRule> Rules;
};
