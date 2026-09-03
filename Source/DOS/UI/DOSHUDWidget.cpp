#include "DOSHUDWidget.h"

#include "Player/DOSPlayerController.h"

void UDOSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 컨트롤러가 이 위젯을 만들지만, 구독은 위젯이 스스로 한다 - 컨트롤러가 위젯 종류마다
	// 어떤 이벤트를 물려야 하는지 알아야 할 이유가 없다.
	if (ADOSPlayerController* PC = Cast<ADOSPlayerController>(GetOwningPlayer()))
	{
		PC->OnInteractPromptChanged.AddUniqueDynamic(this, &UDOSHUDWidget::HandlePromptChanged);
		PC->OnInteractResult.AddUniqueDynamic(this, &UDOSHUDWidget::HandleInteractResult);

		// 위젯이 컨트롤러보다 늦게 붙었으면 이미 대상이 잡혀 있을 수 있다. 그 경우
		// 다음 변화가 올 때까지 안내가 비어 있으므로 지금 값으로 한 번 맞춰 둔다.
		HandlePromptChanged(PC->HasInteractTarget(), PC->GetInteractPrompt());
	}
}

void UDOSHUDWidget::NativeDestruct()
{
	if (ADOSPlayerController* PC = Cast<ADOSPlayerController>(GetOwningPlayer()))
	{
		PC->OnInteractPromptChanged.RemoveDynamic(this, &UDOSHUDWidget::HandlePromptChanged);
		PC->OnInteractResult.RemoveDynamic(this, &UDOSHUDWidget::HandleInteractResult);
	}

	Super::NativeDestruct();
}

void UDOSHUDWidget::HandlePromptChanged(bool bInHasTarget, const FText& InPrompt)
{
	bHasTarget = bInHasTarget;
	Prompt = InPrompt;
	BP_OnPromptChanged(bInHasTarget, InPrompt);
}

void UDOSHUDWidget::HandleInteractResult(EDOSInteractResult Result, FName InterventionId)
{
	BP_OnInteractResult(Result, InterventionId);
}
