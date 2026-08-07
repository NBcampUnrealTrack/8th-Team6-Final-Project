// Resolves which ending the current round's interventions add up to. Evaluates purely against
// UDOSCaseSubsystem::GetActiveInterventions() - clues never factor into ending determination.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DOSEndingSubsystem.generated.h"

class UDOSCaseSubsystem;
class UDOSEndingRuleAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnEndingResolved, FName, EndingId);

UCLASS()
class DOS_API UDOSEndingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Ending")
	TObjectPtr<UDOSEndingRuleAsset> EndingRules;

	// Evaluates EndingRules against the current round's active interventions and returns the
	// highest-priority match, or NAME_None if nothing matches. Does not broadcast.
	UFUNCTION(BlueprintCallable, Category = "DOS|Ending")
	FName EvaluateEnding() const;

	// Evaluates and, if a rule matched, broadcasts OnEndingResolved. Call this when the
	// player concludes the case.
	UFUNCTION(BlueprintCallable, Category = "DOS|Ending")
	FName ResolveEnding();

	UPROPERTY(BlueprintAssignable, Category = "DOS|Ending")
	FDOSOnEndingResolved OnEndingResolved;

private:
	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
};
