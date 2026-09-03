#include "DOSDocumentWidget.h"

#include "Player/DOSPlayerController.h"
#include "Components/Button.h"

void UDOSDocumentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UDOSDocumentWidget::HandleCloseClicked);
	}

	// 키 입력을 받아야 아무 키로나 닫을 수 있다.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void UDOSDocumentWidget::NativeDestruct()
{
	// 위젯이 어떤 경로로 사라지든 시간은 반드시 다시 흘러야 한다.
	CloseDocument();
	Super::NativeDestruct();
}

FReply UDOSDocumentWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	CloseDocument();
	return FReply::Handled();
}

void UDOSDocumentWidget::ShowDocument(const FText& InTitle, const FText& InBody)
{
	Title = InTitle;
	Body = InBody;
	bClosed = false;

	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();

	BP_OnShowDocument(Title, Body);
}

void UDOSDocumentWidget::CloseDocument()
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;

	SetVisibility(ESlateVisibility::Collapsed);

	// 타임라인을 다시 흐르게 하는 것은 컨트롤러의 책임이다.
	if (ADOSPlayerController* PC = GetDOSController())
	{
		PC->CloseDocument();
	}
}

void UDOSDocumentWidget::HandleCloseClicked()
{
	CloseDocument();
}

ADOSPlayerController* UDOSDocumentWidget::GetDOSController() const
{
	return Cast<ADOSPlayerController>(GetOwningPlayer());
}
