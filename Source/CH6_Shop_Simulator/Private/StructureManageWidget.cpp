// Fill out your copyright notice in the Description page of Project Settings.

#include "StructureManageWidget.h"
#include "BuildTile.h"
#include "Components/TextBlock.h"

UStructureManageWidget::UStructureManageWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true; // Q키(취소) 캡처를 위해 여전히 필요
}

void UStructureManageWidget::Setup(ABuildTile* InOwningTile, int32 InRefundAmount)
{
	OwningTile = InOwningTile;
	SelectedIndex = INDEX_NONE;

	if (SellText)
	{
		SellText->SetText(FText::FromString(FString::Printf(TEXT("환불 %dG"), InRefundAmount)));
	}
}

void UStructureManageWidget::OnOptionClicked(int32 OptionIndex)
{
	const int32 OldIndex = SelectedIndex;
	SelectedIndex = OptionIndex;

	// 블루프린트가 이걸 받아서, 새로 선택된 버튼은 하이라이트 켜고
	// 이전에 선택되어 있던 버튼은 하이라이트 꺼주면 됨 (빌드 선택창과 동일한 패턴).
	OnSelectionChanged(SelectedIndex, OldIndex);
}

void UStructureManageWidget::ConfirmSelection()
{
	if (SelectedIndex == INDEX_NONE || !OwningTile)
	{
		return;
	}

	if (SelectedIndex == 2)
	{
		// 판매는 실제로 구현되어 있음
		OwningTile->OnSellSelected();
	}
	else
	{
		// SelectedIndex가 0이면 업그레이드1, 1이면 업그레이드2 - 그대로 인덱스로 씀
		// (아직 미구현이라 블루프린트가 "준비 중입니다" 피드백을 보여주면 됨)
		OnUpgradeNotReady(SelectedIndex);
	}
}

void UStructureManageWidget::CancelMenu()
{
	if (OwningTile)
	{
		OwningTile->OnMenuCancelled();
	}
}

void UStructureManageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsFocusable = true;
	SetKeyboardFocus();
}

FReply UStructureManageWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Q키는 그대로 취소 단축키로 남겨둠 (닫기 버튼 클릭과 동일)
	if (InKeyEvent.GetKey() == EKeys::Q)
	{
		CancelMenu();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}