// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildSelectionWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

UBuildSelectionWidget::UBuildSelectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true; // Q키(취소) 캡처를 위해 여전히 필요
}

void UBuildSelectionWidget::Setup(ABuildTile* InOwningTile, const TArray<FBuildOption>& InOptions)
{
	OwningTile = InOwningTile;
	CachedOptions = InOptions;
	SelectedIndex = INDEX_NONE;

	UButton* Buttons[4] = { TowerButton1, TowerButton2, TowerButton3, TowerButton4 };
	UImage* Icons[4] = { TowerIcon1, TowerIcon2, TowerIcon3, TowerIcon4 };
	UWidget* SlotContainers[4] = { SlotContainer1, SlotContainer2, SlotContainer3, SlotContainer4 };
	UTextBlock* NameTexts[4] = { TowerNameText1, TowerNameText2, TowerNameText3, TowerNameText4 };
	UTextBlock* StatsTexts[4] = { TowerStatsText1, TowerStatsText2, TowerStatsText3, TowerStatsText4 };
	UTextBlock* FlavorTexts[4] = { TowerFlavorText1, TowerFlavorText2, TowerFlavorText3, TowerFlavorText4 };
	UTextBlock* CostTexts[4] = { TowerCostText1, TowerCostText2, TowerCostText3, TowerCostText4 };

	for (int32 i = 0; i < 4; ++i)
	{
		const bool bHasOption = CachedOptions.IsValidIndex(i);

		if (bHasOption)
		{
			const FBuildOption& Option = CachedOptions[i];

			if (Icons[i] && Option.Icon)
			{
				Icons[i]->SetBrushFromTexture(Option.Icon);
			}
			if (NameTexts[i])
			{
				NameTexts[i]->SetText(FText::FromString(Option.Label));
			}
			if (StatsTexts[i])
			{
				StatsTexts[i]->SetText(Option.StatsDescription);
			}
			if (FlavorTexts[i])
			{
				FlavorTexts[i]->SetText(Option.FlavorText);
			}
			if (CostTexts[i])
			{
				CostTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%d G"), Option.Cost)));
			}
		}

		// ★ SlotContainer(버튼+아이콘+텍스트+꾸밈 요소를 다 감싼 것)를 만들어뒀으면
		// 그것만 껐다 켜면 됨. 안 만들었으면 개별 위젯들을 각자 숨김.
		if (SlotContainers[i])
		{
			SlotContainers[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		else
		{
			if (Icons[i]) Icons[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (Buttons[i]) Buttons[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (NameTexts[i]) NameTexts[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (StatsTexts[i]) StatsTexts[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (FlavorTexts[i]) FlavorTexts[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (CostTexts[i]) CostTexts[i]->SetVisibility(bHasOption ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UBuildSelectionWidget::OnClickOption(int32 OptionIndex)
{
	const int32 OldIndex = SelectedIndex;
	SelectedIndex = OptionIndex;

	// 블루프린트가 이걸 받아서, 새로 선택된 슬롯은 하이라이트 켜고
	// 이전에 선택되어 있던 슬롯은 하이라이트 꺼주면 됨.
	OnSelectionChanged(SelectedIndex, OldIndex);
}

void UBuildSelectionWidget::ConfirmSelection()
{
	if (SelectedIndex != INDEX_NONE && OwningTile)
	{
		OwningTile->OnOptionSelected(SelectedIndex);
	}
}

void UBuildSelectionWidget::CancelSelection()
{
	if (OwningTile)
	{
		OwningTile->OnMenuCancelled();
	}
}

void UBuildSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsFocusable = true;
	SetKeyboardFocus();
}

FReply UBuildSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Q키는 그대로 취소 단축키로 남겨둠 (None! 버튼 클릭과 동일)
	if (InKeyEvent.GetKey() == EKeys::Q)
	{
		CancelSelection();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}