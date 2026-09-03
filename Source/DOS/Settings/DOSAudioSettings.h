// Project-wide audio asset wiring, edited in Project Settings -> Game -> DOS Audio.
// Separate from UDOSAudioSubsystem (which does the applying) because only UDeveloperSettings
// subclasses get their own Project Settings page; a subsystem's UPROPERTYs don't show up there.
//
// Any slot left empty is skipped at runtime, so the volume sliders still save correctly before
// the audio assets exist - they just don't affect anything audible yet.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DOSAudioSettings.generated.h"

class USoundClass;
class USoundMix;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "DOS Audio"))
class DOS_API UDOSAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	// Carries the per-class volume overrides. Required for any volume setting to be audible.
	UPROPERTY(EditAnywhere, Config, Category = "DOS|Audio")
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	UPROPERTY(EditAnywhere, Config, Category = "DOS|Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, Config, Category = "DOS|Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, Config, Category = "DOS|Audio")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	UPROPERTY(EditAnywhere, Config, Category = "DOS|Audio")
	TSoftObjectPtr<USoundClass> VoiceSoundClass;
};
