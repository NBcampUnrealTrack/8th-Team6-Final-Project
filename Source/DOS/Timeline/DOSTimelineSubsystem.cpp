#include "DOSTimelineSubsystem.h"
#include "Engine/GameInstance.h"
#include "Stats/Stats.h"

void UDOSTimelineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CurrentTimeSeconds = 0.f;
	PlaybackRate = 1.f;
	CurrentRound = 1;
	PlayState = EDOSTimelinePlayState::Paused;
}

void UDOSTimelineSubsystem::Tick(float DeltaTime)
{
	if (PlayState == EDOSTimelinePlayState::Paused)
	{
		return;
	}

	const float Direction = (PlayState == EDOSTimelinePlayState::Rewinding) ? -1.f : 1.f;
	const float NewTime = FMath::Clamp(CurrentTimeSeconds + DeltaTime * PlaybackRate * Direction, 0.f, TimelineDurationSeconds);
	SetCurrentTime(NewTime);

	// Playback simply stops at either end of the loop. A new round only begins when
	// RestartLoop() is called explicitly (e.g. from a "watch again" UI action).
	const bool bHitEnd = PlayState == EDOSTimelinePlayState::Playing && NewTime >= TimelineDurationSeconds;
	const bool bHitStart = PlayState == EDOSTimelinePlayState::Rewinding && NewTime <= 0.f;
	if (bHitEnd || bHitStart)
	{
		Pause();
	}
}

bool UDOSTimelineSubsystem::IsTickable() const
{
	return !IsTemplate();
}

TStatId UDOSTimelineSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDOSTimelineSubsystem, STATGROUP_Tickables);
}

UWorld* UDOSTimelineSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

void UDOSTimelineSubsystem::Play()
{
	PlayState = EDOSTimelinePlayState::Playing;
}

void UDOSTimelineSubsystem::Pause()
{
	PlayState = EDOSTimelinePlayState::Paused;
}

void UDOSTimelineSubsystem::Rewind()
{
	PlayState = EDOSTimelinePlayState::Rewinding;
}

void UDOSTimelineSubsystem::SetPlaybackRate(float NewRate)
{
	PlaybackRate = FMath::Max(NewRate, 0.f);
}

void UDOSTimelineSubsystem::ScrubTo(float NewTimeSeconds)
{
	SetCurrentTime(FMath::Clamp(NewTimeSeconds, 0.f, TimelineDurationSeconds));
}

void UDOSTimelineSubsystem::RestartLoop()
{
	++CurrentRound;
	PlayState = EDOSTimelinePlayState::Paused;
	SetCurrentTime(0.f);
	OnRoundRestarted.Broadcast(CurrentRound);
}

void UDOSTimelineSubsystem::RestoreState(float TimeSeconds, int32 Round)
{
	CurrentRound = FMath::Max(Round, 1);
	PlayState = EDOSTimelinePlayState::Paused;
	CurrentTimeSeconds = FMath::Clamp(TimeSeconds, 0.f, TimelineDurationSeconds);
	OnTimelineTimeChanged.Broadcast(CurrentTimeSeconds);
}

void UDOSTimelineSubsystem::SetCurrentTime(float NewTimeSeconds)
{
	if (FMath::IsNearlyEqual(CurrentTimeSeconds, NewTimeSeconds))
	{
		return;
	}

	CurrentTimeSeconds = NewTimeSeconds;
	OnTimelineTimeChanged.Broadcast(CurrentTimeSeconds);
}
