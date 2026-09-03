#include "DOSMainMenuWidget.h"
#include "DOSSettingsWidget.h"
#include "SaveGame/DOSSaveGameSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"

void UDOSMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddDynamic(this, &UDOSMainMenuWidget::HandleNewGameClicked);
	}

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UDOSMainMenuWidget::HandleContinueClicked);

		const UDOSSaveGameSubsystem* SaveGameSubsystem = GetSaveGameSubsystem();
		const bool bHasSave = SaveGameSubsystem && SaveGameSubsystem->HasSaveGame();
		ContinueButton->SetIsEnabled(bHasSave);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UDOSMainMenuWidget::HandleSettingsClicked);
	}

	if (BackgroundImage)
	{
		const UDOSSaveGameSubsystem* SaveGameSubsystem = GetSaveGameSubsystem();
		const bool bTrueEnding = SaveGameSubsystem && SaveGameSubsystem->HasTrueEndingUnlocked();
		const TSoftObjectPtr<UTexture2D>& BackgroundToShow = bTrueEnding ? TrueEndingBackground : DefaultBackground;

		if (UTexture2D* Texture = BackgroundToShow.LoadSynchronous())
		{
			BackgroundImage->SetBrushFromTexture(Texture);
		}
	}
}

void UDOSMainMenuWidget::HandleNewGameClicked()
{
	if (UDOSSaveGameSubsystem* SaveGameSubsystem = GetSaveGameSubsystem())
	{
		SaveGameSubsystem->StartNewGame();
	}
}

void UDOSMainMenuWidget::HandleContinueClicked()
{
	if (UDOSSaveGameSubsystem* SaveGameSubsystem = GetSaveGameSubsystem())
	{
		SaveGameSubsystem->ContinueGame();
	}
}

void UDOSMainMenuWidget::HandleSettingsClicked()
{
	if (!SettingsWidgetClass)
	{
		return;
	}

	if (UDOSSettingsWidget* SettingsWidget = CreateWidget<UDOSSettingsWidget>(GetOwningPlayer(), SettingsWidgetClass))
	{
		SettingsWidget->AddToViewport();
	}
}

UDOSSaveGameSubsystem* UDOSMainMenuWidget::GetSaveGameSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UDOSSaveGameSubsystem>() : nullptr;
}
