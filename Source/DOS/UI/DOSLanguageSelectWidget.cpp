#include "DOSLanguageSelectWidget.h"
#include "Settings/DOSGameUserSettings.h"
#include "Components/Button.h"
#include "Engine/Engine.h"

void UDOSLanguageSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (EnglishButton)
	{
		EnglishButton->OnClicked.AddDynamic(this, &UDOSLanguageSelectWidget::HandleEnglishClicked);
	}
	if (JapaneseButton)
	{
		JapaneseButton->OnClicked.AddDynamic(this, &UDOSLanguageSelectWidget::HandleJapaneseClicked);
	}
	if (ChineseSimplifiedButton)
	{
		ChineseSimplifiedButton->OnClicked.AddDynamic(this, &UDOSLanguageSelectWidget::HandleChineseSimplifiedClicked);
	}
	if (ChineseTraditionalButton)
	{
		ChineseTraditionalButton->OnClicked.AddDynamic(this, &UDOSLanguageSelectWidget::HandleChineseTraditionalClicked);
	}
	if (KoreanButton)
	{
		KoreanButton->OnClicked.AddDynamic(this, &UDOSLanguageSelectWidget::HandleKoreanClicked);
	}

	// OS 언어를 지원 목록으로 접어 기본 선택으로 삼는다. 한국어 윈도우에서 켰는데 커서가
	// English에 있으면 매번 옮겨야 하고, 이 화면은 아직 아무 언어로도 번역되지 않은 상태다.
	DefaultCultureCode = UDOSGameUserSettings::GetSystemDefaultCultureCode();
	BP_OnDefaultLanguageResolved(DefaultCultureCode);
}

void UDOSLanguageSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 포커스는 위젯이 실제로 화면에 올라온 뒤에 줘야 잡힌다.
	SetIsFocusable(true);
	if (UButton* Default = GetDefaultButton())
	{
		Default->SetKeyboardFocus();
	}
	else
	{
		SetKeyboardFocus();
	}
}

UButton* UDOSLanguageSelectWidget::GetDefaultButton() const
{
	if (DefaultCultureCode == TEXT("ko"))      { return KoreanButton; }
	if (DefaultCultureCode == TEXT("ja"))      { return JapaneseButton; }
	if (DefaultCultureCode == TEXT("zh-Hans")) { return ChineseSimplifiedButton; }
	if (DefaultCultureCode == TEXT("zh-Hant")) { return ChineseTraditionalButton; }
	return EnglishButton;
}

void UDOSLanguageSelectWidget::HandleEnglishClicked()
{
	ConfirmLanguage(TEXT("en"));
}

void UDOSLanguageSelectWidget::HandleJapaneseClicked()
{
	ConfirmLanguage(TEXT("ja"));
}

void UDOSLanguageSelectWidget::HandleChineseSimplifiedClicked()
{
	ConfirmLanguage(TEXT("zh-Hans"));
}

void UDOSLanguageSelectWidget::HandleChineseTraditionalClicked()
{
	ConfirmLanguage(TEXT("zh-Hant"));
}

void UDOSLanguageSelectWidget::HandleKoreanClicked()
{
	ConfirmLanguage(TEXT("ko"));
}

void UDOSLanguageSelectWidget::ConfirmLanguage(const FString& CultureCode)
{
	if (UDOSGameUserSettings* Settings = Cast<UDOSGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr))
	{
		Settings->SetLanguage(CultureCode); // applies the culture and saves immediately
	}

	OnLanguageConfirmed.Broadcast(CultureCode);
}
