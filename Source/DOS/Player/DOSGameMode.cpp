#include "DOSGameMode.h"

#include "DOSObserverCharacter.h"
#include "DOSPlayerController.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/GameInstance.h"

ADOSGameMode::ADOSGameMode()
{
	DefaultPawnClass = ADOSObserverCharacter::StaticClass();
	PlayerControllerClass = ADOSPlayerController::StaticClass();
}

void ADOSGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDOSTimelineSubsystem* Timeline = GI->GetSubsystem<UDOSTimelineSubsystem>())
		{
			Timeline->SetPlaybackRate(InitialPlaybackRate);
			if (bAutoPlayTimeline)
			{
				Timeline->Play();
			}
		}
	}
}
