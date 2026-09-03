// Title screen widget: New Game / Continue buttons routed through UDOSSaveGameSubsystem, and a
// background image that swaps to the true-ending variant once it's been unlocked. Bind the
// optional widgets below by giving them the same names in the WBP_MainMenu widget tree.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DOSMainMenuWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UDOSSaveGameSubsystem;
class UDOSSettingsWidget;

UCLASS()
class DOS_API UDOSMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Shown when no true ending has been unlocked yet.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|MainMenu")
	TSoftObjectPtr<UTexture2D> DefaultBackground;

	// Shown once the player has reached the true ending at least once, forever after.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|MainMenu")
	TSoftObjectPtr<UTexture2D> TrueEndingBackground;

	// Spawned on top of the menu when SettingsButton is clicked. Set to WBP_Settings.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|MainMenu")
	TSubclassOf<UDOSSettingsWidget> SettingsWidgetClass;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|MainMenu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|MainMenu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|MainMenu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|MainMenu", meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

private:
	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UDOSSaveGameSubsystem* GetSaveGameSubsystem() const;
};
