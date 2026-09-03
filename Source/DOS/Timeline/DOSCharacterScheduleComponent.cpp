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
		Result.WitnessId = Only.WitnessId;
		Result.GoalLocation = Only.Location;
		Result.GoalRotation = Only.Rotation;
		Result.GoalTimeSeconds = Only.TimeSeconds;
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
		Result.WitnessId = Last.WitnessId;
		Result.GoalLocation = Last.Location;
		Result.GoalRotation = Last.Rotation;
		Result.GoalTimeSeconds = Last.TimeSeconds;
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
	Result.WitnessId = Prev.WitnessId;
	// **다음 웨이포인트가 목표다.** Prev를 목표로 삼으면 이미 지나온 자리로 가려 해서
	// 출발이 한 박자 늦고, 다음 박자가 올 때까지 도착하지 못한다. 웹 검증의 기준도
	// "이 박자에 출발해 다음 지시 전에 닿는가"이므로 Next가 맞다.
	Result.GoalLocation = Next.Location;
	Result.GoalRotation = Next.Rotation;
	Result.GoalTimeSeconds = Next.TimeSeconds;
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
	// through a movement component - unless the owner drives its own walking (NavMesh).
	if (bApplyTransformToOwner)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->SetActorLocationAndRotation(State.Location, State.Rotation);
		}
	}

	OnCharacterStateApplied.Broadcast(State);
}
