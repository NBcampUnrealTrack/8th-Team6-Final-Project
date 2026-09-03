// Options screen: graphics, sound, general game toggles and language, all backed by
// UDOSGameUserSettings. Same BindWidgetOptional convention as UDOSMainMenuWidget - give the
// widgets in WBP_Settings these exact names and they wire themselves up; leave any of them out
// and that control is simply absent.
//
// Sound / subtitles / language save the instant they change (no Apply step). Resolution,
// quality and fullscreen deliberately do NOT - they go through ApplySettings/ConfirmVideoMode
// via ApplyButton, so a display mode the monitor can't show auto-reverts instead of leaving the
// player stuck on a black screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DOSSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UDOSGameUserSettings;

UCLASS()
class DOS_API UDOSSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	// ---------- graphics ----------
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResolutionComboBox;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> QualityComboBox;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> FullscreenCheckBox;

	// Confirms pending graphics changes. Sound/subtitles/language ignore this - they're already
	// saved by the time it's clicked.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ApplyButton;

	// ---------- sound ----------
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<USlider> SfxVolumeSlider;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<USlider> VoiceVolumeSlider;

	// ---------- game ----------
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> SubtitlesCheckBox;

	// ---------- language ----------
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> LanguageComboBox;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Settings", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

private:
	void PopulateResolutions();
	void PopulateQualityLevels();
	void PopulateLanguages();
	void SyncWidgetsToSettings();

	UFUNCTION()
	void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleFullscreenChanged(bool bIsChecked);

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleSfxVolumeChanged(float Value);

	UFUNCTION()
	void HandleVoiceVolumeChanged(float Value);

	UFUNCTION()
	void HandleSubtitlesChanged(bool bIsChecked);

	UFUNCTION()
	void HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCloseClicked();

	UDOSGameUserSettings* GetSettings() const;

	// Guards the OnValueChanged handlers while SyncWidgetsToSettings() is populating the widgets,
	// so programmatic setup doesn't write the value straight back (and re-save) as if the player
	// had moved the control.
	bool bSyncingWidgets = false;
};
