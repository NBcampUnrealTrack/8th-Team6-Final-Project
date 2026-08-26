// Ending result screen: narration lines, then per-character epilogues, then a return-to-menu
// button. Each line/epilogue auto-advances after AutoAdvanceSeconds, or immediately on a click
// or key press. Content comes from the matched FDOSEndingRule - call InitializeForEnding right
// after spawning this widget. Visual presentation (text/portrait display) is left to Blueprint
// via the BP_On... events; only the return button is a plain C++-bound widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Case/DOSEndingRuleAsset.h"
#include "Engine/TimerHandle.h"
#include "DOSEndingResultWidget.generated.h"

class UButton;
class UDOSEndingRuleAsset;

UCLASS()
class DOS_API UDOSEndingResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Ending")
	float AutoAdvanceSeconds = 4.f;

	// Looks up EndingId in EndingRules and starts the narration -> epilogue -> button sequence.
	UFUNCTION(BlueprintCallable, Category = "DOS|Ending")
	void InitializeForEnding(FName EndingId, const UDOSEndingRuleAsset* EndingRules);

	// Fired once per narration line, in order. Blueprint should display Line as the centered
	// narration text.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Ending")
	void BP_OnShowNarrationLine(const FText& Line);

	// Fired once per character epilogue, in order, after all narration lines are done.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Ending")
	void BP_OnShowCharacterEpilogue(const FDOSCharacterEpilogue& Epilogue);

	// Fired once, after the last epilogue, right before the return button becomes visible.
	UFUNCTION(BlueprintImplementableEvent, Category = "DOS|Ending")
	void BP_OnAllContentShown();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// Hidden (Collapsed) until all narration/epilogue content has been shown.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Ending", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnToMenuButton;

private:
	enum class EDOSEndingResultStage : uint8
	{
		Narration,
		Epilogue,
		Complete
	};

	void ShowCurrent();
	void AdvanceNext();
	void TryAdvanceEarly();

	UFUNCTION()
	void HandleReturnToMenuClicked();

	TArray<FText> NarrationLines;
	TArray<FDOSCharacterEpilogue> CharacterEpilogues;

	EDOSEndingResultStage CurrentStage = EDOSEndingResultStage::Narration;
	int32 CurrentIndex = 0;

	FTimerHandle AdvanceTimerHandle;
};
