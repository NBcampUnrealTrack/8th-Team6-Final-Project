#include "DOSMainMenuGameMode.h"
#include "DOSMainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

ADOSMainMenuGameMode::ADOSMainMenuGameMode()
{
	// The menu level has no gameplay actors - skip pawn spawning so it doesn't need a
	// PlayerStart.
	DefaultPawnClass = nullptr;
}

void ADOSMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

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

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget ? MenuWidget->TakeWidget() : TSharedPtr<SWidget>());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
