// Local device/user preferences: graphics (resolution/quality/fullscreen - inherited from
// UGameUserSettings, nothing new needed), sound (master/music/sfx/voice volume), a couple of
// general game toggles, and the player's chosen UI language. Deliberately separate from
// UDOSSaveGame (SaveGame/DOSSaveGame.h), which holds game-progress data, not local preferences.
//
// Every setter here that isn't graphics-related saves immediately (SetX -> apply -> SaveConfig,
// no separate "Apply" step) so a change sticks even if the player closes the settings screen or
// the game crashes right after. Graphics (resolution/fullscreen) is the one exception - it goes
// through the inherited ApplySettings()/ConfirmVideoMode() flow so a bad resolution auto-reverts
// instead of locking the player out, matching Epic's own UGameUserSettings convention.
//
// Set this as the active class via Config/DefaultEngine.ini's
// [/Script/Engine.Engine] GameUserSettingsClassName=/Script/DOS.DOSGameUserSettings

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "DOSGameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDOSOnAudioSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnLanguageChanged, const FString&, NewCultureCode);

UCLASS()
class DOS_API UDOSGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	// ---------- sound (0-1, linear) ----------
	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Sound")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Sound")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Sound")
	void SetSfxVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Sound")
	void SetVoiceVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Sound")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Sound")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Sound")
	float GetSfxVolume() const { return SfxVolume; }

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Sound")
	float GetVoiceVolume() const { return VoiceVolume; }

	// Fires after any of the four volume setters above so UDOSAudioSubsystem can re-apply them
	// to the live SoundClass mix.
	UPROPERTY(BlueprintAssignable, Category = "DOS|Settings|Sound")
	FDOSOnAudioSettingsChanged OnAudioSettingsChanged;

	// ---------- general game toggles ----------
	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Game")
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Game")
	bool AreSubtitlesEnabled() const { return bSubtitlesEnabled; }

	// ---------- language ----------
	// One of "en", "ja", "zh-Hans", "zh-Hant", "ko" - matches Engine culture codes directly so
	// no extra mapping table is needed.
	UFUNCTION(BlueprintCallable, Category = "DOS|Settings|Language")
	void SetLanguage(const FString& NewCultureCode);

	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Language")
	FString GetLanguage() const { return SelectedCultureCode; }

	// False until SetLanguage() has been called at least once - DOSMainMenuGameMode uses this to
	// decide whether to show the first-launch language picker before the main menu.
	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Language")
	bool HasSelectedLanguage() const { return bHasSelectedLanguage; }

	// OS 언어를 이 게임이 지원하는 다섯 개 중 하나로 접는다.
	//   en / ja / zh-Hans / zh-Hant / ko   (모르는 언어는 en)
	//
	// 첫 실행 언어 선택 화면의 **기본 선택**에 쓴다. 한국어 윈도우에서 켰는데 커서가
	// English에 있으면 매번 옮겨야 하고, 정작 그 화면은 아직 아무 언어로도 번역되지 않은
	// 상태다(버튼 글자가 각자의 언어로 쓰여 있는 이유가 그것이다).
	//
	// 중국어는 지역만으로 갈린다 - zh-TW/zh-HK/zh-MO와 Hant 표기는 번체, 나머지는 간체.
	UFUNCTION(BlueprintPure, Category = "DOS|Settings|Language")
	static FString GetSystemDefaultCultureCode();

	UPROPERTY(BlueprintAssignable, Category = "DOS|Settings|Language")
	FDOSOnLanguageChanged OnLanguageChanged;

	// Applies SelectedCultureCode via FInternationalization - called both at startup (if a
	// language was already chosen) and from SetLanguage(). Safe to call with an empty/invalid
	// code (no-ops).
	void ApplySavedLanguage();

	// Reapplies the saved language every time settings are (re)loaded, so the very first
	// LoadSettings() call at engine startup already has the right culture active before any UI
	// shows - not just from the second session onward.
	virtual void LoadSettings(bool bForceReload = false) override;

protected:
	UPROPERTY(config)
	float MasterVolume = 1.f;

	UPROPERTY(config)
	float MusicVolume = 1.f;

	UPROPERTY(config)
	float SfxVolume = 1.f;

	UPROPERTY(config)
	float VoiceVolume = 1.f;

	UPROPERTY(config)
	bool bSubtitlesEnabled = true;

	UPROPERTY(config)
	FString SelectedCultureCode;

	UPROPERTY(config)
	bool bHasSelectedLanguage = false;
};
