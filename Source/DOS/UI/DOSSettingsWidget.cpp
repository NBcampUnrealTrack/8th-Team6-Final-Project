#include "DOSSettingsWidget.h"
#include "Settings/DOSGameUserSettings.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	// Culture code <-> display label, in the same order as the combo box entries. Labels are
	// written in their own language on purpose (same reasoning as the first-launch picker) so
	// they stay readable no matter which language is currently active.
	struct FDOSLanguageOption
	{
		const TCHAR* CultureCode;
		const TCHAR* DisplayName;
	};

	const FDOSLanguageOption GLanguageOptions[] = {
		{ TEXT("en"), TEXT("English") },
		{ TEXT("ja"), TEXT("日本語") },
		{ TEXT("zh-Hans"), TEXT("简体中文") },
		{ TEXT("zh-Hant"), TEXT("繁體中文") },
		{ TEXT("ko"), TEXT("한국어") },
	};

	const TCHAR* GQualityLabels[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };

	FString ResolutionToString(const FIntPoint& Resolution)
	{
		return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
	}
}

void UDOSSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PopulateResolutions();
	PopulateQualityLevels();
	PopulateLanguages();

	if (ResolutionComboBox)
	{
		ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &UDOSSettingsWidget::HandleResolutionChanged);
	}
	if (QualityComboBox)
	{
		QualityComboBox->OnSelectionChanged.AddDynamic(this, &UDOSSettingsWidget::HandleQualityChanged);
	}
	if (FullscreenCheckBox)
	{
		FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UDOSSettingsWidget::HandleFullscreenChanged);
	}
	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &UDOSSettingsWidget::HandleApplyClicked);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UDOSSettingsWidget::HandleMasterVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UDOSSettingsWidget::HandleMusicVolumeChanged);
	}
	if (SfxVolumeSlider)
	{
		SfxVolumeSlider->OnValueChanged.AddDynamic(this, &UDOSSettingsWidget::HandleSfxVolumeChanged);
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->OnValueChanged.AddDynamic(this, &UDOSSettingsWidget::HandleVoiceVolumeChanged);
	}
	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->OnCheckStateChanged.AddDynamic(this, &UDOSSettingsWidget::HandleSubtitlesChanged);
	}
	if (LanguageComboBox)
	{
		LanguageComboBox->OnSelectionChanged.AddDynamic(this, &UDOSSettingsWidget::HandleLanguageChanged);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UDOSSettingsWidget::HandleCloseClicked);
	}

	SyncWidgetsToSettings();
}

void UDOSSettingsWidget::PopulateResolutions()
{
	if (!ResolutionComboBox)
	{
		return;
	}

	ResolutionComboBox->ClearOptions();

	TArray<FIntPoint> Resolutions;
	if (UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions))
	{
		for (const FIntPoint& Resolution : Resolutions)
		{
			ResolutionComboBox->AddOption(ResolutionToString(Resolution));
		}
	}
}

void UDOSSettingsWidget::PopulateQualityLevels()
{
	if (!QualityComboBox)
	{
		return;
	}

	QualityComboBox->ClearOptions();
	for (const TCHAR* Label : GQualityLabels)
	{
		QualityComboBox->AddOption(Label);
	}
}

void UDOSSettingsWidget::PopulateLanguages()
{
	if (!LanguageComboBox)
	{
		return;
	}

	LanguageComboBox->ClearOptions();
	for (const FDOSLanguageOption& Option : GLanguageOptions)
	{
		LanguageComboBox->AddOption(Option.DisplayName);
	}
}

void UDOSSettingsWidget::SyncWidgetsToSettings()
{
	const UDOSGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}

	TGuardValue<bool> SyncGuard(bSyncingWidgets, true);

	if (ResolutionComboBox)
	{
		ResolutionComboBox->SetSelectedOption(ResolutionToString(Settings->GetScreenResolution()));
	}
	if (QualityComboBox)
	{
		const int32 Quality = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, UE_ARRAY_COUNT(GQualityLabels) - 1);
		QualityComboBox->SetSelectedOption(GQualityLabels[Quality]);
	}
	if (FullscreenCheckBox)
	{
		FullscreenCheckBox->SetIsChecked(Settings->GetFullscreenMode() != EWindowMode::Windowed);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(Settings->GetMusicVolume());
	}
	if (SfxVolumeSlider)
	{
		SfxVolumeSlider->SetValue(Settings->GetSfxVolume());
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->SetValue(Settings->GetVoiceVolume());
	}
	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->SetIsChecked(Settings->AreSubtitlesEnabled());
	}
	if (LanguageComboBox)
	{
		const FString Current = Settings->GetLanguage();
		for (const FDOSLanguageOption& Option : GLanguageOptions)
		{
			if (Current == Option.CultureCode)
			{
				LanguageComboBox->SetSelectedOption(Option.DisplayName);
				break;
			}
		}
	}
}

// ---------- graphics (staged until Apply) ----------

void UDOSSettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bSyncingWidgets)
	{
		return;
	}

	UDOSGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}

	FString WidthString, HeightString;
	if (!SelectedItem.Split(TEXT(" x "), &WidthString, &HeightString))
	{
		return;
	}

	Settings->SetScreenResolution(FIntPoint(FCString::Atoi(*WidthString), FCString::Atoi(*HeightString)));
}

void UDOSSettingsWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bSyncingWidgets)
	{
		return;
	}

	UDOSGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(GQualityLabels); ++Index)
	{
		if (SelectedItem == GQualityLabels[Index])
		{
			Settings->SetOverallScalabilityLevel(Index);
			break;
		}
	}
}

void UDOSSettingsWidget::HandleFullscreenChanged(bool bIsChecked)
{
	if (bSyncingWidgets)
	{
		return;
	}

	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetFullscreenMode(bIsChecked ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	}
}

void UDOSSettingsWidget::HandleApplyClicked()
{
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		// ApplySettings(false) runs the video-mode confirmation path, which reverts on its own if
		// the new mode never gets confirmed, and saves once the change sticks.
		Settings->ApplySettings(false);
		SyncWidgetsToSettings();
	}
}

// ---------- sound / game / language (saved immediately) ----------

void UDOSSettingsWidget::HandleMasterVolumeChanged(float Value)
{
	if (bSyncingWidgets)
	{
		return;
	}
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMasterVolume(Value);
	}
}

void UDOSSettingsWidget::HandleMusicVolumeChanged(float Value)
{
	if (bSyncingWidgets)
	{
		return;
	}
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetMusicVolume(Value);
	}
}

void UDOSSettingsWidget::HandleSfxVolumeChanged(float Value)
{
	if (bSyncingWidgets)
	{
		return;
	}
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetSfxVolume(Value);
	}
}

void UDOSSettingsWidget::HandleVoiceVolumeChanged(float Value)
{
	if (bSyncingWidgets)
	{
		return;
	}
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetVoiceVolume(Value);
	}
}

void UDOSSettingsWidget::HandleSubtitlesChanged(bool bIsChecked)
{
	if (bSyncingWidgets)
	{
		return;
	}
	if (UDOSGameUserSettings* Settings = GetSettings())
	{
		Settings->SetSubtitlesEnabled(bIsChecked);
	}
}

void UDOSSettingsWidget::HandleLanguageChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bSyncingWidgets)
	{
		return;
	}

	UDOSGameUserSettings* Settings = GetSettings();
	if (!Settings)
	{
		return;
	}

	for (const FDOSLanguageOption& Option : GLanguageOptions)
	{
		if (SelectedItem == Option.DisplayName)
		{
			Settings->SetLanguage(Option.CultureCode);
			break;
		}
	}
}

void UDOSSettingsWidget::HandleCloseClicked()
{
	RemoveFromParent();
}

UDOSGameUserSettings* UDOSSettingsWidget::GetSettings() const
{
	return Cast<UDOSGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr);
}
