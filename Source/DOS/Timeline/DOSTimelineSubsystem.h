// Master timeline controller for a single 30-minute case. Owns the current playback time,
// round count and play state; every other system reacts to this rather than tracking its own
// notion of time.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "DOSTimelineTypes.h"
#include "DOSTimelineSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnTimelineTimeChanged, float, NewTimeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnRoundRestarted, int32, NewRound);
// 배속이 바뀌었다. 화면에 잠깐 띄웠다 사라지는 표시가 여기에 붙는다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnPlaybackRateChanged, float, NewRate);

UCLASS()
class DOS_API UDOSTimelineSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void Pause();

	// Starts continuous playback backward at the current playback rate.
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void Rewind();

	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void SetPlaybackRate(float NewRate);

	// Jumps directly to a point in the timeline (e.g. UI scrub bar). Does NOT start a new
	// round or touch ActiveInterventions - use this for reviewing the current round.
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void ScrubTo(float NewTimeSeconds);

	// Resets time to 0 and begins a new observation round. Broadcasts OnRoundRestarted, which
	// UDOSCaseSubsystem uses to clear this round's ActiveInterventions.
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void RestartLoop();

	// 일시정지 화면의 「처음부터」. **회차 번호를 올리지 않는다** - 지금 회차를 다시 하는
	// 것이지 다음 회차로 넘어가는 것이 아니다.
	//
	// ⚠ RestartLoop()을 여기에 쓰면 안 된다. 그쪽은 회차를 올리므로, 1회차에서 「처음부터」를
	// 누르면 관찰도 안 한 채 2회차가 되어 **개입 권한이 열려버린다.**
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void RestartRound();

	// Directly sets time/round from save data (e.g. Continue) and unconditionally broadcasts
	// OnTimelineTimeChanged, unlike ScrubTo/SetCurrentTime which skip the broadcast when the
	// new value is unchanged - listeners bound after this call still need to hear about it.
	UFUNCTION(BlueprintCallable, Category = "DOS|Timeline")
	void RestoreState(float TimeSeconds, int32 Round);

	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	float GetCurrentTime() const { return CurrentTimeSeconds; }

	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	float GetDuration() const { return TimelineDurationSeconds; }

	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	EDOSTimelinePlayState GetPlayState() const { return PlayState; }

	UPROPERTY(BlueprintAssignable, Category = "DOS|Timeline")
	FDOSOnTimelineTimeChanged OnTimelineTimeChanged;

	UFUNCTION(BlueprintPure, Category = "DOS|Timeline")
	float GetPlaybackRate() const { return PlaybackRate; }

	UPROPERTY(BlueprintAssignable, Category = "DOS|Timeline")
	FDOSOnRoundRestarted OnRoundRestarted;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Timeline")
	FDOSOnPlaybackRateChanged OnPlaybackRateChanged;

	virtual void Deinitialize() override;

protected:
	// Length of the case timeline. 1800s = 30 minutes.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Timeline")
	float TimelineDurationSeconds = 1800.f;

private:
	void SetCurrentTime(float NewTimeSeconds);

	// **배속은 월드 전체의 시간 배율로 건다.** 시계만 곱하면 NPC는 실시간으로 걸어서,
	// 배속을 올리는 순간 다음 지시가 내려올 때까지 목적지에 닿지 못한다(화면에서는 계속
	// 끌려다니기만 한다). 웹 프로토타입은 "배속을 올리면 시계와 걸음이 같은 배수로
	// 빨라진다"를 전제로 32조합을 검증했으므로, UE5도 같은 방식이어야 그 검증이 유효하다.
	// 정지 중에는 1로 되돌린다 - 멈춰 있는데 월드만 8배로 돌 이유가 없다.
	void ApplyTimeDilation();

	float CurrentTimeSeconds = 0.f;
	float PlaybackRate = 1.f;
	int32 CurrentRound = 1;
	EDOSTimelinePlayState PlayState = EDOSTimelinePlayState::Paused;
};
