// Game mode for the L_MainMenu level: spawns the title screen widget and switches input to
// UI-only so the player can click its buttons. Set MenuWidgetClass to WBP_MainMenu in the
// Blueprint defaults, and set this class as L_MainMenu's Game Mode Override in World Settings.
//
// On the very first launch (before any language has been chosen) the language picker is shown
// instead, and the main menu only appears once a language has been confirmed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DOSMainMenuGameMode.generated.h"

class UDOSMainMenuWidget;
class UDOSLanguageSelectWidget;

UCLASS()
class DOS_API ADOSMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADOSMainMenuGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "DOS|MainMenu")
	TSubclassOf<UDOSMainMenuWidget> MenuWidgetClass;

	// Shown before the main menu on first launch only. Leave unset to always go straight to the
	// main menu (the saved language, if any, still applies).
	UPROPERTY(EditDefaultsOnly, Category = "DOS|MainMenu")
	TSubclassOf<UDOSLanguageSelectWidget> LanguageSelectWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleLanguageConfirmed(const FString& CultureCode);

	void ShowLanguageSelect();
	void ShowMainMenu();
	void FocusWidget(UUserWidget* Widget);

	UPROPERTY(Transient)
	TObjectPtr<UDOSMainMenuWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDOSLanguageSelectWidget> LanguageSelectWidget;
};
