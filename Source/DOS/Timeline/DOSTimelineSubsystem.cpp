#include "DOSTimelineSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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

	// **DeltaTime에 PlaybackRate를 곱하지 않는다.** 배속은 월드 시간 배율로 걸려 있으므로
	// 여기 들어오는 DeltaTime이 이미 배속만큼 커져 있다. 다시 곱하면 제곱이 된다.
	const float Direction = (PlayState == EDOSTimelinePlayState::Rewinding) ? -1.f : 1.f;
	const float NewTime = FMath::Clamp(CurrentTimeSeconds + DeltaTime * Direction, 0.f, TimelineDurationSeconds);
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

void UDOSTimelineSubsystem::Deinitialize()
{
	// 레벨을 떠날 때 배율을 원복하지 않으면 메인메뉴가 8배로 돈다.
	if (const UWorld* World = GetTickableGameObjectWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}
	Super::Deinitialize();
}

void UDOSTimelineSubsystem::Play()
{
	PlayState = EDOSTimelinePlayState::Playing;
	ApplyTimeDilation();
}

void UDOSTimelineSubsystem::Pause()
{
	PlayState = EDOSTimelinePlayState::Paused;
	ApplyTimeDilation();
}

void UDOSTimelineSubsystem::Rewind()
{
	PlayState = EDOSTimelinePlayState::Rewinding;
	ApplyTimeDilation();
}

void UDOSTimelineSubsystem::SetPlaybackRate(float NewRate)
{
	const float Clamped = FMath::Clamp(NewRate, 0.1f, 16.f);
	if (FMath::IsNearlyEqual(PlaybackRate, Clamped))
	{
		return;
	}

	PlaybackRate = Clamped;
	ApplyTimeDilation();
	OnPlaybackRateChanged.Broadcast(PlaybackRate);
}

void UDOSTimelineSubsystem::ApplyTimeDilation()
{
	const UWorld* World = GetTickableGameObjectWorld();
	if (!World)
	{
		return;
	}

	const float Dilation = (PlayState == EDOSTimelinePlayState::Paused) ? 1.f : PlaybackRate;
	UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
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

void UDOSTimelineSubsystem::RestartRound()
{
	// 회차 번호는 그대로 둔다. 개입은 OnRoundRestarted를 들은 UDOSCaseSubsystem이 지운다.
	PlayState = EDOSTimelinePlayState::Paused;
	ApplyTimeDilation();
	SetCurrentTime(0.f);
	OnTimelineTimeChanged.Broadcast(0.f);
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
