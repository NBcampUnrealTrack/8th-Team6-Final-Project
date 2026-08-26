// Game mode for the L_MainMenu level: spawns the title screen widget and switches input to
// UI-only so the player can click its buttons. Set MenuWidgetClass to WBP_MainMenu in the
// Blueprint defaults, and set this class as L_MainMenu's Game Mode Override in World Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DOSMainMenuGameMode.generated.h"

class UDOSMainMenuWidget;

UCLASS()
class DOS_API ADOSMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADOSMainMenuGameMode();

	UPROPERTY(EditDefaultsOnly, Category = "DOS|MainMenu")
	TSubclassOf<UDOSMainMenuWidget> MenuWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDOSMainMenuWidget> MenuWidget;
};
