// Attached to any interactable object that can change the outcome of the case (a gas valve,
// a wine bottle, ...). Only usable within its configured time window; a successful use
// registers the intervention with UDOSCaseSubsystem for this round.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DOSInterventionComponent.generated.h"

class UDOSTimelineSubsystem;
class UDOSCaseSubsystem;

UCLASS(ClassGroup = (DOS), meta = (BlueprintSpawnableComponent))
class DOS_API UDOSInterventionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDOSInterventionComponent();

	// Key this intervention is tracked/matched against in ending rules and schedule overrides.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	FName InterventionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	float WindowStartSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	float WindowEndSeconds = 1800.f;

	// Attempts to register the intervention. Fails silently (returns false) if the current
	// timeline time is outside the window or it was already triggered this round.
	UFUNCTION(BlueprintCallable, Category = "DOS|Intervention")
	bool TryIntervene();

	UFUNCTION(BlueprintPure, Category = "DOS|Intervention")
	bool IsWithinWindow() const;

	UFUNCTION(BlueprintPure, Category = "DOS|Intervention")
	bool IsActiveThisRound() const;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
};
