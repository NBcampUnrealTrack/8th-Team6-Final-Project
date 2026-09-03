#include "DOSMainMenuGameMode.h"
#include "DOSMainMenuWidget.h"
#include "DOSLanguageSelectWidget.h"
#include "Settings/DOSGameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

ADOSMainMenuGameMode::ADOSMainMenuGameMode()
{
	// The menu level has no gameplay actors - skip pawn spawning so it doesn't need a
	// PlayerStart.
	DefaultPawnClass = nullptr;
}

void ADOSMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	const UDOSGameUserSettings* Settings = Cast<UDOSGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr);
	const bool bNeedsLanguagePick = LanguageSelectWidgetClass && Settings && !Settings->HasSelectedLanguage();

	if (bNeedsLanguagePick)
	{
		ShowLanguageSelect();
	}
	else
	{
		ShowMainMenu();
	}
}

void ADOSMainMenuGameMode::ShowLanguageSelect()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	LanguageSelectWidget = CreateWidget<UDOSLanguageSelectWidget>(PlayerController, LanguageSelectWidgetClass);
	if (!LanguageSelectWidget)
	{
		ShowMainMenu(); // never strand the player on an empty screen
		return;
	}

	LanguageSelectWidget->OnLanguageConfirmed.AddDynamic(this, &ADOSMainMenuGameMode::HandleLanguageConfirmed);
	LanguageSelectWidget->AddToViewport();
	FocusWidget(LanguageSelectWidget);
}

void ADOSMainMenuGameMode::HandleLanguageConfirmed(const FString& CultureCode)
{
	if (LanguageSelectWidget)
	{
		LanguageSelectWidget->OnLanguageConfirmed.RemoveDynamic(this, &ADOSMainMenuGameMode::HandleLanguageConfirmed);
		LanguageSelectWidget->RemoveFromParent();
		LanguageSelectWidget = nullptr;
	}

	ShowMainMenu();
}

void ADOSMainMenuGameMode::ShowMainMenu()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !MenuWidgetClass)
	{
		return;
	}

	MenuWidget = CreateWidget<UDOSMainMenuWidget>(PlayerController, MenuWidgetClass);
	if (MenuWidget)
	{
		MenuWidget->AddToViewport();
	}

	FocusWidget(MenuWidget);
}

void ADOSMainMenuGameMode::FocusWidget(UUserWidget* Widget)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget ? Widget->TakeWidget() : TSharedPtr<SWidget>());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
