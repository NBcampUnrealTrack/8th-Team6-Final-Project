#include "DOSPauseWidget.h"

#include "DOSSettingsWidget.h"
#include "Case/DOSCaseSubsystem.h"
#include "Settings/DOSLevelSettings.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	template <typename T>
	T* GetDOSSubsystem(const UWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<T>() : nullptr;
	}
}

void UDOSPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)       { ResumeButton->OnClicked.AddUniqueDynamic(this, &UDOSPauseWidget::HandleResume); }
	if (RewindButton)       { RewindButton->OnClicked.AddUniqueDynamic(this, &UDOSPauseWidget::HandleRewind); }
	if (RestartRoundButton) { RestartRoundButton->OnClicked.AddUniqueDynamic(this, &UDOSPauseWidget::HandleRestartRound); }
	if (SettingsButton)     { SettingsButton->OnClicked.AddUniqueDynamic(this, &UDOSPauseWidget::HandleSettings); }
	if (MainMenuButton)     { MainMenuButton->OnClicked.AddUniqueDynamic(this, &UDOSPauseWidget::HandleMainMenu); }
}

bool UDOSPauseWidget::CanRewindToLastIntervention() const
{
	const UDOSCaseSubsystem* Case = GetDOSSubsystem<UDOSCaseSubsystem>(this);
	return Case && Case->GetLastInterventionTime() >= 0.f;
}

void UDOSPauseWidget::OpenPause()
{
	// 타임라인을 세운다. 배속도 1로 돌아간다(ApplyTimeDilation이 정지 상태를 본다).
	if (UDOSTimelineSubsystem* Timeline = GetDOSSubsystem<UDOSTimelineSubsystem>(this))
	{
		Timeline->Pause();
	}

	SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	BP_OnRefreshState(CanRewindToLastIntervention());
}

void UDOSPauseWidget::ClosePause()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	if (UDOSTimelineSubsystem* Timeline = GetDOSSubsystem<UDOSTimelineSubsystem>(this))
	{
		Timeline->Play();
	}
}

void UDOSPauseWidget::HandleResume()
{
	ClosePause();
}

void UDOSPauseWidget::HandleRewind()
{
	UDOSCaseSubsystem* Case = GetDOSSubsystem<UDOSCaseSubsystem>(this);
	if (!Case || !Case->RewindToLastIntervention())
	{
		return; // 되돌릴 개입이 없다. 화면을 닫지 않는다.
	}
	ClosePause();
}

void UDOSPauseWidget::HandleRestartRound()
{
	// ⚠ RestartLoop()이 아니다. 그쪽은 회차를 올려서 1회차 관찰을 건너뛰게 만든다.
	if (UDOSTimelineSubsystem* Timeline = GetDOSSubsystem<UDOSTimelineSubsystem>(this))
	{
		Timeline->RestartRound();
	}
	ClosePause();
}

void UDOSPauseWidget::HandleSettings()
{
	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DOS] 일시정지: Settings Widget Class가 비어 있습니다."));
		return;
	}

	if (UDOSSettingsWidget* Settings = CreateWidget<UDOSSettingsWidget>(GetOwningPlayer(), SettingsWidgetClass))
	{
		Settings->AddToViewport();
	}
}

void UDOSPauseWidget::HandleMainMenu()
{
	// 게임을 떠나므로 시간 배율을 원복한다 - 안 하면 메인메뉴가 배속으로 돈다.
	if (UDOSTimelineSubsystem* Timeline = GetDOSSubsystem<UDOSTimelineSubsystem>(this))
	{
		Timeline->Pause();
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeUIOnly());
		PC->SetShowMouseCursor(true);
	}

	// 레벨은 Project Settings -> Game -> DOS Levels 에서 지정한다.
	const UDOSLevelSettings* Settings = UDOSLevelSettings::Get();
	if (!Settings || Settings->MainMenuLevel.IsNull())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DOS] Main Menu Level이 비어 있습니다 - Project Settings > Game > DOS Levels 에서 지정하세요."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->MainMenuLevel);
}
