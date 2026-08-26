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

	UPROPERTY(BlueprintAssignable, Category = "DOS|Timeline")
	FDOSOnRoundRestarted OnRoundRestarted;

protected:
	// Length of the case timeline. 1800s = 30 minutes.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Timeline")
	float TimelineDurationSeconds = 1800.f;

private:
	void SetCurrentTime(float NewTimeSeconds);

	float CurrentTimeSeconds = 0.f;
	float PlaybackRate = 1.f;
	int32 CurrentRound = 1;
	EDOSTimelinePlayState PlayState = EDOSTimelinePlayState::Paused;
};
