// Applies the volume values stored in UDOSGameUserSettings to the actual SoundClass mix.
// Split from UDOSGameUserSettings because SetSoundMixClassOverride needs a world context, which
// a UGameUserSettings (a plain config object, not world-bound) doesn't have.
//
// Which SoundMix/SoundClass assets to drive comes from UDOSAudioSettings (Project Settings ->
// Game -> DOS Audio); any slot left empty there is skipped, so audio work can land incrementally
// without breaking startup.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DOSAudioSubsystem.generated.h"

class USoundClass;
class USoundMix;

UCLASS()
class DOS_API UDOSAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Pushes every current volume value from UDOSGameUserSettings onto its SoundClass. Called
	// after each map load and whenever OnAudioSettingsChanged fires.
	UFUNCTION(BlueprintCallable, Category = "DOS|Audio")
	void ApplyVolumeSettings();

private:
	UFUNCTION()
	void HandleAudioSettingsChanged();

	void HandlePostLoadMap(UWorld* LoadedWorld);

	void ApplyOne(USoundMix* Mix, const TSoftObjectPtr<USoundClass>& SoundClass, float Volume);
};
