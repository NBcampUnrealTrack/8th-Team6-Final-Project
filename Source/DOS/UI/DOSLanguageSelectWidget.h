// First-launch language picker, shown by ADOSMainMenuGameMode before the main menu when
// UDOSGameUserSettings::HasSelectedLanguage() is false.
//
// Each button's label must be written in its OWN language in the Blueprint (English / 日本語 /
// 简体中文 / 繁體中文 / 한국어) and must NOT be a localized FText - the whole point of this screen
// is that the player can't yet read whatever language happens to be active.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DOSLanguageSelectWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDOSOnLanguageConfirmed, const FString&, CultureCode);

UCLASS()
class DOS_API UDOSLanguageSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Fired once the player picks a language and it has been saved. ADOSMainMenuGameMode listens
	// for this to swap in the main menu.
	UPROPERTY(BlueprintAssignable, Category = "DOS|Language")
	FDOSOnLanguageConfirmed OnLanguageConfirmed;

	// OS 언어에서 접어낸 기본 선택. en / ja / zh-Hans / zh-Hant / ko 중 하나다.
	// 해당 버튼에 **키보드 포커스가 미리 가 있으므로** 엔터만 눌러도 그 언어로 시작한다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language")
	FString DefaultCultureCode;

	// 어느 버튼을 강조할지 블루프린트에 알린다. 테두리·색으로 표시하면 된다 -
	// 포커스 표시만으로는 눈에 잘 안 띈다.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Language")
	void BP_OnDefaultLanguageResolved(const FString& CultureCode);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	// DefaultCultureCode에 해당하는 버튼. 없으면 English.
	UButton* GetDefaultButton() const;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EnglishButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language", meta = (BindWidgetOptional))
	TObjectPtr<UButton> JapaneseButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChineseSimplifiedButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ChineseTraditionalButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Language", meta = (BindWidgetOptional))
	TObjectPtr<UButton> KoreanButton;

private:
	UFUNCTION()
	void HandleEnglishClicked();

	UFUNCTION()
	void HandleJapaneseClicked();

	UFUNCTION()
	void HandleChineseSimplifiedClicked();

	UFUNCTION()
	void HandleChineseTraditionalClicked();

	UFUNCTION()
	void HandleKoreanClicked();

	void ConfirmLanguage(const FString& CultureCode);
};
