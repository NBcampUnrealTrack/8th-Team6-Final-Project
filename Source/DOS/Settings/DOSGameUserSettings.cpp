#include "DOSGameUserSettings.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

void UDOSGameUserSettings::SetMasterVolume(float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	SaveSettings();
	OnAudioSettingsChanged.Broadcast();
}

void UDOSGameUserSettings::SetMusicVolume(float NewVolume)
{
	MusicVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	SaveSettings();
	OnAudioSettingsChanged.Broadcast();
}

void UDOSGameUserSettings::SetSfxVolume(float NewVolume)
{
	SfxVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	SaveSettings();
	OnAudioSettingsChanged.Broadcast();
}

void UDOSGameUserSettings::SetVoiceVolume(float NewVolume)
{
	VoiceVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	SaveSettings();
	OnAudioSettingsChanged.Broadcast();
}

void UDOSGameUserSettings::SetSubtitlesEnabled(bool bEnabled)
{
	bSubtitlesEnabled = bEnabled;
	SaveSettings();
}

FString UDOSGameUserSettings::GetSystemDefaultCultureCode()
{
	// OS가 알려주는 것을 쓴다. 엔진이 이미 다른 문화권으로 바뀌어 있어도 이 값은
	// 운영체제 설정 그대로다.
	FString Os = FPlatformMisc::GetDefaultLanguage();
	if (Os.IsEmpty())
	{
		const FCultureRef Current = FInternationalization::Get().GetCurrentLanguage();
		Os = Current->GetName();
	}

	Os = Os.ToLower().Replace(TEXT("_"), TEXT("-"));

	if (Os.StartsWith(TEXT("ko"))) { return TEXT("ko"); }
	if (Os.StartsWith(TEXT("ja"))) { return TEXT("ja"); }
	if (Os.StartsWith(TEXT("zh")))
	{
		// 번체를 쓰는 곳: 대만·홍콩·마카오, 그리고 Hant를 명시한 경우.
		const bool bTraditional =
			Os.Contains(TEXT("hant")) ||
			Os.Contains(TEXT("-tw")) ||
			Os.Contains(TEXT("-hk")) ||
			Os.Contains(TEXT("-mo"));
		return bTraditional ? TEXT("zh-Hant") : TEXT("zh-Hans");
	}

	// 지원하지 않는 언어는 영어로 떨어뜨린다.
	return TEXT("en");
}

void UDOSGameUserSettings::SetLanguage(const FString& NewCultureCode)
{
	if (NewCultureCode.IsEmpty())
	{
		return;
	}

	SelectedCultureCode = NewCultureCode;
	bHasSelectedLanguage = true;
	ApplySavedLanguage();
	SaveSettings();
	OnLanguageChanged.Broadcast(SelectedCultureCode);
}

void UDOSGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	ApplySavedLanguage();
}

void UDOSGameUserSettings::ApplySavedLanguage()
{
	if (SelectedCultureCode.IsEmpty())
	{
		return;
	}

	// SetCurrentCulture affects both display formatting (dates/numbers) and which localized
	// FText variant resolves; SetCurrentLanguage alone would leave formatting on the OS default.
	// Fails silently (returns false, leaves the previous culture active) if the engine has no
	// localization data for this code yet - e.g. before a translator has added zh-Hant via the
	// Localization Dashboard - rather than crashing on an unrecognized culture.
	FInternationalization::Get().SetCurrentCulture(SelectedCultureCode);
}
