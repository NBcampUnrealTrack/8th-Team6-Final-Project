#include "DOSInterventionComponent.h"
#include "DOSCaseSubsystem.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UDOSInterventionComponent::UDOSInterventionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

bool UDOSInterventionComponent::TryIntervene()
{
	if (InterventionId.IsNone() || !IsWithinWindow())
	{
		return false;
	}

	UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	return Case && Case->RegisterIntervention(InterventionId);
}
