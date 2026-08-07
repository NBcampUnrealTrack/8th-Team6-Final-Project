#include "DOSCharacterScheduleComponent.h"
#include "DOSCharacterScheduleAsset.h"
#include "DOSTimelineSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UDOSCharacterScheduleComponent::UDOSCharacterScheduleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDOSCharacterScheduleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			CachedTimelineSubsystem = GameInstance->GetSubsystem<UDOSTimelineSubsystem>();
		}
	}

	RefreshResolvedSchedule(TSet<FName>());

	if (UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get())
	{
		Timeline->OnTimelineTimeChanged.AddDynamic(this, &UDOSCharacterScheduleComponent::HandleTimelineTimeChanged);
		HandleTimelineTimeChanged(Timeline->GetCurrentTime());
	}
}

void UDOSCharacterScheduleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get())
	{
		Timeline->OnTimelineTimeChanged.RemoveDynamic(this, &UDOSCharacterScheduleComponent::HandleTimelineTimeChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UDOSCharacterScheduleComponent::RefreshResolvedSchedule(const TSet<FName>& ActiveInterventions)
{
	ResolvedWaypoints.Reset();
	if (ScheduleAsset)
	{
		ScheduleAsset->BuildResolvedWaypoints(ActiveInterventions, ResolvedWaypoints);
	}
}

FDOSCharacterState UDOSCharacterScheduleComponent::SampleAtTime(float TimeSeconds) const
{
	FDOSCharacterState Result;

	if (ResolvedWaypoints.Num() == 0)
	{
		return Result;
	}

	if (ResolvedWaypoints.Num() == 1 || TimeSeconds <= ResolvedWaypoints[0].TimeSeconds)
	{
		const FDOSWaypoint& Only = ResolvedWaypoints[0];
		Result.Location = Only.Location;
		Result.Rotation = Only.Rotation;
		Result.FloorId = Only.FloorId;
		Result.RoomId = Only.RoomId;
		Result.ActionTag = Only.ActionTag;
		Result.ActionMontage = Only.ActionMontage;
		Result.bIsClamped = true;
		return Result;
	}

	if (TimeSeconds >= ResolvedWaypoints.Last().TimeSeconds)
	{
		const FDOSWaypoint& Last = ResolvedWaypoints.Last();
		Result.Location = Last.Location;
		Result.Rotation = Last.Rotation;
		Result.FloorId = Last.FloorId;
		Result.RoomId = Last.RoomId;
		Result.ActionTag = Last.ActionTag;
		Result.ActionMontage = Last.ActionMontage;
		Result.bIsClamped = true;
		return Result;
	}

	// Binary search for the first waypoint strictly after T; Prev is the one just before it.
	int32 Low = 0;
	int32 High = ResolvedWaypoints.Num() - 1;
	while (Low < High)
	{
		const int32 Mid = (Low + High) / 2;
		if (ResolvedWaypoints[Mid].TimeSeconds <= TimeSeconds)
		{
			Low = Mid + 1;
		}
		else
		{
			High = Mid;
		}
	}

	const FDOSWaypoint& Next = ResolvedWaypoints[Low];
	const FDOSWaypoint& Prev = ResolvedWaypoints[Low - 1];

	const float Span = Next.TimeSeconds - Prev.TimeSeconds;
	const float Alpha = Span > KINDA_SMALL_NUMBER ? (TimeSeconds - Prev.TimeSeconds) / Span : 1.f;

	Result.Location = FMath::Lerp(Prev.Location, Next.Location, Alpha);
	Result.Rotation = FQuat::Slerp(Prev.Rotation.Quaternion(), Next.Rotation.Quaternion(), Alpha).Rotator();
	Result.FloorId = Prev.FloorId;
	Result.RoomId = Prev.RoomId;
	Result.ActionTag = Prev.ActionTag;
	Result.ActionMontage = Prev.ActionMontage;
	Result.bIsClamped = false;

	return Result;
}

void UDOSCharacterScheduleComponent::HandleTimelineTimeChanged(float NewTimeSeconds)
{
	ApplyState(SampleAtTime(NewTimeSeconds));
}

void UDOSCharacterScheduleComponent::ApplyState(const FDOSCharacterState& State)
{
	// Schedule playback is fully scripted, so the transform is set directly rather than
	// through a movement component.
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocationAndRotation(State.Location, State.Rotation);
	}

	OnCharacterStateApplied.Broadcast(State);
}
