#include "DOSEndingResultWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

namespace DOSEndingResultConstants
{
	static const TCHAR* MainMenuLevelPath = TEXT("/Game/DoS/Maps/L_MainMenu");
}

void UDOSEndingResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ReturnToMenuButton)
	{
		ReturnToMenuButton->SetVisibility(ESlateVisibility::Collapsed);
		ReturnToMenuButton->OnClicked.AddDynamic(this, &UDOSEndingResultWidget::HandleReturnToMenuClicked);
	}

	SetIsFocusable(true);
	SetFocus();
}

FReply UDOSEndingResultWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TryAdvanceEarly();
	return FReply::Handled();
}

FReply UDOSEndingResultWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TryAdvanceEarly();
	return FReply::Handled();
}

void UDOSEndingResultWidget::InitializeForEnding(FName EndingId, const UDOSEndingRuleAsset* EndingRules)
{
	NarrationLines.Reset();
	CharacterEpilogues.Reset();

	if (EndingRules)
	{
		const FDOSEndingRule* MatchedRule = EndingRules->Rules.FindByPredicate([EndingId](const FDOSEndingRule& Rule)
		{
			return Rule.EndingId == EndingId;
		});

		if (MatchedRule)
		{
			NarrationLines = MatchedRule->NarrationLines;
			CharacterEpilogues = MatchedRule->CharacterEpilogues;
		}
	}

	CurrentStage = EDOSEndingResultStage::Narration;
	CurrentIndex = 0;
	ShowCurrent();
}

void UDOSEndingResultWidget::ShowCurrent()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AdvanceTimerHandle);
	}

	if (CurrentStage == EDOSEndingResultStage::Narration)
	{
		if (CurrentIndex >= NarrationLines.Num())
		{
			CurrentStage = EDOSEndingResultStage::Epilogue;
			CurrentIndex = 0;
			ShowCurrent();
			return;
		}

		BP_OnShowNarrationLine(NarrationLines[CurrentIndex]);
	}
	else if (CurrentStage == EDOSEndingResultStage::Epilogue)
	{
		if (CurrentIndex >= CharacterEpilogues.Num())
		{
			CurrentStage = EDOSEndingResultStage::Complete;

			if (ReturnToMenuButton)
			{
				ReturnToMenuButton->SetVisibility(ESlateVisibility::Visible);
			}

			BP_OnAllContentShown();
			return;
		}

		BP_OnShowCharacterEpilogue(CharacterEpilogues[CurrentIndex]);
	}
	else
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AdvanceTimerHandle, this, &UDOSEndingResultWidget::AdvanceNext, AutoAdvanceSeconds, false);
	}
}

void UDOSEndingResultWidget::AdvanceNext()
{
	++CurrentIndex;
	ShowCurrent();
}

void UDOSEndingResultWidget::TryAdvanceEarly()
{
	if (CurrentStage == EDOSEndingResultStage::Complete)
	{
		return;
	}

	AdvanceNext();
}

void UDOSEndingResultWidget::HandleReturnToMenuClicked()
{
	UGameplayStatics::OpenLevel(this, FName(DOSEndingResultConstants::MainMenuLevelPath));
}
