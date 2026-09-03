#include "DOSAudioSubsystem.h"
#include "DOSAudioSettings.h"
#include "DOSGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UDOSGameUserSettings* GetDOSSettings()
	{
		return Cast<UDOSGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr);
	}
}

void UDOSAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UDOSGameUserSettings* Settings = GetDOSSettings())
	{
		Settings->OnAudioSettingsChanged.AddDynamic(this, &UDOSAudioSubsystem::HandleAudioSettingsChanged);
	}

	// Subsystems initialize before any world exists, and the GameplayStatics sound calls need a
	// world context - so the saved volumes are (re)applied after each map load instead of here,
	// otherwise the first level would silently start at whatever the mix defaults to. Same
	// delegate UDOSSaveGameSubsystem uses for its own post-load work.
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDOSAudioSubsystem::HandlePostLoadMap);
}

void UDOSAudioSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (UDOSGameUserSettings* Settings = GetDOSSettings())
	{
		Settings->OnAudioSettingsChanged.RemoveDynamic(this, &UDOSAudioSubsystem::HandleAudioSettingsChanged);
	}

	Super::Deinitialize();
}

void UDOSAudioSubsystem::HandleAudioSettingsChanged()
{
	ApplyVolumeSettings();
}

void UDOSAudioSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	ApplyVolumeSettings();
}

void UDOSAudioSubsystem::ApplyVolumeSettings()
{
	const UDOSGameUserSettings* Settings = GetDOSSettings();
	const UDOSAudioSettings* AudioSettings = GetDefault<UDOSAudioSettings>();
	if (!Settings || !AudioSettings)
	{
		return;
	}

	USoundMix* Mix = AudioSettings->SettingsSoundMix.LoadSynchronous();
	if (!Mix)
	{
		return; // no mix assigned yet - volumes are still saved, just not audible
	}

	ApplyOne(Mix, AudioSettings->MasterSoundClass, Settings->GetMasterVolume());
	ApplyOne(Mix, AudioSettings->MusicSoundClass, Settings->GetMusicVolume());
	ApplyOne(Mix, AudioSettings->SfxSoundClass, Settings->GetSfxVolume());
	ApplyOne(Mix, AudioSettings->VoiceSoundClass, Settings->GetVoiceVolume());

	// Pushing the mix is what actually makes the per-class overrides above audible.
	UGameplayStatics::PushSoundMixModifier(this, Mix);
}

void UDOSAudioSubsystem::ApplyOne(USoundMix* Mix, const TSoftObjectPtr<USoundClass>& SoundClass, float Volume)
{
	USoundClass* Class = SoundClass.LoadSynchronous();
	if (!Class)
	{
		return; // unassigned slot - audio for this bus isn't set up yet
	}

	// FadeInTime 0 so dragging a volume slider tracks the pointer instead of lagging behind it.
	UGameplayStatics::SetSoundMixClassOverride(this, Mix, Class, Volume, /*Pitch=*/1.f, /*FadeInTime=*/0.f, /*bApplyToChildren=*/true);
}
