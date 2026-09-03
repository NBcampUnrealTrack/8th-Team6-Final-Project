#include "DOSInterventionComponent.h"
#include "DOSCaseSubsystem.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "DOSIntervention"

UDOSInterventionComponent::UDOSInterventionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PromptText = LOCTEXT("DefaultPrompt", "밀기");
}

void UDOSInterventionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			CachedTimelineSubsystem = GameInstance->GetSubsystem<UDOSTimelineSubsystem>();
			CachedCaseSubsystem = GameInstance->GetSubsystem<UDOSCaseSubsystem>();
		}
	}
}

bool UDOSInterventionComponent::IsWithinWindow() const
{
	const UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get();
	if (!Timeline)
	{
		return false;
	}

	const float CurrentTime = Timeline->GetCurrentTime();
	return CurrentTime >= WindowStartSeconds && CurrentTime <= WindowEndSeconds;
}

bool UDOSInterventionComponent::IsActiveThisRound() const
{
	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	return Case && Case->HasIntervened(InterventionId);
}

EDOSInteractResult UDOSInterventionComponent::Interact()
{
	return TryIntervene() ? EDOSInteractResult::Succeeded : EDOSInteractResult::Rejected;
}

bool UDOSInterventionComponent::IsUnlocked() const
{
	const UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get();
	if (!Timeline || Timeline->GetCurrentRound() < 2)
	{
		return false; // 1회차는 관찰 전용이다.
	}

	if (RequiredWitnessId.IsNone())
	{
		return true;
	}

	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	return Case && Case->HasDiscoveredClue(RequiredWitnessId);
}

bool UDOSInterventionComponent::TryIntervene()
{
	if (InterventionId.IsNone() || !IsWithinWindow() || !IsUnlocked())
	{
		return false;
	}

	UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	if (!Case || !Case->RegisterIntervention(InterventionId))
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[DOS] 개입 성립: %s (%s)"),
		*InterventionId.ToString(), *GetOwner()->GetName());

	OnIntervened.Broadcast();
	return true;
}

#undef LOCTEXT_NAMESPACE
