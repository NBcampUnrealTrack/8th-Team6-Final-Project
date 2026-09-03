#include "Player/DOSPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "Case/DOSInterventionComponent.h"
#include "Case/DOSReadableComponent.h"
#include "UI/DOSDocumentWidget.h"
#include "UI/DOSPauseWidget.h"
#include "UI/DOSHUDWidget.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
	// 안내 문구를 다시 계산하는 주기(초).
	constexpr float FocusRefreshInterval = 0.1f;
}

void ADOSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!MappingContext)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DOS] %s: Mapping Context가 비어 있습니다 - 키가 아무 액션에도 닿지 않습니다. ")
			TEXT("컨트롤러 Class Defaults의 DOS|Input에 IMC_Player를 꽂으세요."),
			*GetName());
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[DOS] %s: EnhancedInput 서브시스템을 못 얻었습니다."), *GetName());
		return;
	}

	Subsystem->AddMappingContext(MappingContext, 0);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FocusTimer, this, &ADOSPlayerController::RefreshFocus, FocusRefreshInterval, true);
	}

	// 위젯은 한 번만 만들어 붙여두고 숨긴다.
	if (DocumentWidgetClass)
	{
		DocumentWidget = CreateWidget<UDOSDocumentWidget>(this, DocumentWidgetClass);
		if (DocumentWidget)
		{
			DocumentWidget->AddToViewport(10);
			DocumentWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (PauseWidgetClass)
	{
		PauseWidget = CreateWidget<UDOSPauseWidget>(this, PauseWidgetClass);
		if (PauseWidget)
		{
			PauseWidget->AddToViewport(20); // 문서보다 위
			PauseWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// HUD는 상시 표시라 숨기지 않는다. 안내를 넣고 빼는 것은 위젯 안에서 한다 -
	// 여기서 위젯 전체를 접으면 나중에 다른 상시 표시(자막 등)를 붙일 자리가 없어진다.
	// Z=0: 문서·일시정지 아래에 깔린다.
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UDOSHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: HUDWidgetClass가 비어 있습니다 - 상호작용 키 안내가 아예 안 뜹니다. ")
			TEXT("BP_DOSPlayerController의 DOS|UI에서 WBP_HUD를 지정하세요."),
			*GetName());
	}
}

void ADOSPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bShowTimelineDebug || !GEngine)
	{
		return;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UDOSTimelineSubsystem* Timeline =
		GameInstance ? GameInstance->GetSubsystem<UDOSTimelineSubsystem>() : nullptr;
	if (!Timeline)
	{
		return;
	}

	// 이야기 시각은 분:초로 읽는다 - 대본과 스케줄이 전부 그 표기다(20:10, 21:10 ...).
	const int32 Total = FMath::FloorToInt(Timeline->GetCurrentTime());
	const float Rate = Timeline->GetPlaybackRate();

	const TCHAR* StateText = TEXT("재생");
	switch (Timeline->GetPlayState())
	{
	case EDOSTimelinePlayState::Paused:    StateText = TEXT("정지"); break;
	case EDOSTimelinePlayState::Rewinding: StateText = TEXT("되감기"); break;
	default: break;
	}

	// 키를 고정하면 줄이 쌓이지 않고 제자리에서 갱신된다.
	GEngine->AddOnScreenDebugMessage(
		/*Key*/ 1001, /*TimeToDisplay*/ 0.f, FColor::Yellow,
		FString::Printf(TEXT("[DOS] %02d:%02d   x%.2g   %s   %d회차"),
			Total / 60, Total % 60, Rate, StateText, Timeline->GetCurrentRound()));

	// 지금 잡히는 대상. 안내가 왜 안 뜨는지 볼 때 쓴다.
	FString FocusText = TEXT("-");
	if (const UDOSReadableComponent* Doc = FocusedDocument.Get())
	{
		FocusText = FString::Printf(TEXT("문서 %s"), *Doc->DocumentId.ToString());
	}
	else if (const UDOSInterventionComponent* Obj = FocusedInteractable.Get())
	{
		FocusText = FString::Printf(TEXT("개입 %s"), *Obj->InterventionId.ToString());
	}
	GEngine->AddOnScreenDebugMessage(
		1002, 0.f, FColor::Silver, FString::Printf(TEXT("[DOS] 대상: %s"), *FocusText));
}

void ADOSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ADOSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DOS] %s: InputComponent가 EnhancedInputComponent가 아닙니다(%s). ")
			TEXT("Project Settings > Input > Default Input Component Class를 확인하세요."),
			*GetName(),
			InputComponent ? *InputComponent->GetClass()->GetName() : TEXT("null"));
		return;
	}

	// 액션 칸을 비워두면 키는 들어오는데 아무 일도 안 일어난다. 조용히 넘어가지 않고 남긴다.
	if (!MoveAction || !LookAction)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DOS] %s: %s%s이(가) 비어 있습니다 - 그래서 움직이지 않습니다."),
			*GetName(),
			MoveAction ? TEXT("") : TEXT("Move Action "),
			LookAction ? TEXT("") : TEXT("Look Action"));
	}

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADOSPlayerController::Move);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADOSPlayerController::Look);
	}
	if (InteractAction)
	{
		// 누르는 순간 한 번만. Triggered로 걸면 누르고 있는 동안 계속 들어온다.
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ADOSPlayerController::Interact);
	}
	if (SpeedAction)
	{
		Input->BindAction(SpeedAction, ETriggerEvent::Triggered, this, &ADOSPlayerController::ChangeSpeed);
	}
	if (PauseAction)
	{
		Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ADOSPlayerController::TogglePause);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: Interact Action이 비어 있습니다 - 오브젝트를 밀 수 없습니다."), *GetName());
	}
}

void ADOSPlayerController::Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return; // 빙의 전이거나 폰이 죽었다.
	}

	const FVector2D MoveInput = Value.Get<FVector2D>();

	// 폰이 bUseControllerRotationYaw로 시선을 따라 돌기 때문에, 액터의 앞/오른쪽이
	// 곧 바라보는 방향이다. Pitch/Roll은 폰에 안 걸리므로 벡터가 이미 수평이다.
	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		// 캐릭터가 바라보는 방향(정면)으로 X축 이동
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), MoveInput.X);
	}

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		// 캐릭터의 오른쪽 방향으로 Y축 이동
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), MoveInput.Y);
	}
}

void ADOSPlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	AddYawInput(Axis.X * LookSensitivity);
	AddPitchInput(Axis.Y * LookSensitivity);
}

void ADOSPlayerController::ChangeSpeed(const FInputActionValue& Value)
{
	const float Wheel = Value.Get<float>();
	if (FMath::IsNearlyZero(Wheel) || SpeedSteps.Num() == 0)
	{
		return;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UDOSTimelineSubsystem* Timeline = GameInstance ? GameInstance->GetSubsystem<UDOSTimelineSubsystem>() : nullptr;
	if (!Timeline)
	{
		return;
	}

	// 지금 배속이 프리셋 어디에 있는지 찾는다. 정확히 일치하지 않으면 가장 가까운 것.
	const float Current = Timeline->GetPlaybackRate();
	int32 Index = 0;
	float BestDelta = TNumericLimits<float>::Max();
	for (int32 i = 0; i < SpeedSteps.Num(); ++i)
	{
		const float Delta = FMath::Abs(SpeedSteps[i] - Current);
		if (Delta < BestDelta)
		{
			BestDelta = Delta;
			Index = i;
		}
	}

	const int32 NextIndex = FMath::Clamp(Index + (Wheel > 0.f ? 1 : -1), 0, SpeedSteps.Num() - 1);
	if (NextIndex != Index)
	{
		// 표시는 SetPlaybackRate가 쏘는 OnPlaybackRateChanged에 위젯이 붙는다.
		Timeline->SetPlaybackRate(SpeedSteps[NextIndex]);
	}
}

// --- 상호작용 -----------------------------------------------------------------

template <typename TComponent>
TComponent* ADOSPlayerController::FindBestComponent() const
{
	const APawn* ControlledPawn = GetPawn();
	const UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return nullptr;
	}

	const FVector Origin = ControlledPawn->GetActorLocation();
	const FVector ViewDir = GetControlRotation().Vector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(InteractHalfAngleDegrees));

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DOSInteract), /*bTraceComplex*/ false, ControlledPawn);
	World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		// AllStaticObjects로 좁히면 안 된다 - 블루프린트로 만든 개입 오브젝트는 Movable이라
		// WorldDynamic이 되고, 그러면 조용히 안 잡힌다.
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		FCollisionShape::MakeSphere(InteractRange),
		Params);

	TComponent* Best = nullptr;
	float BestScore = -1.f;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor)
		{
			continue;
		}
		TComponent* Found = Actor->FindComponentByClass<TComponent>();
		if (!Found)
		{
			continue;
		}

		const FVector ToTarget = Actor->GetActorLocation() - Origin;
		const float Distance = ToTarget.Size();
		if (Distance > InteractRange)
		{
			continue;
		}

		// 시선에 얼마나 맞는가. 등 뒤의 물건은 버린다.
		const float Dot = Distance > KINDA_SMALL_NUMBER
			? FVector::DotProduct(ViewDir, ToTarget / Distance)
			: 1.f;
		if (Dot < MinDot)
		{
			continue;
		}

		// 시선 정렬을 우선하고 거리로 가른다.
		const float Score = Dot - (Distance / InteractRange) * 0.25f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Found;
		}
	}

	return Best;
}

UDOSInterventionComponent* ADOSPlayerController::FindBestInteractable() const
{
	return FindBestComponent<UDOSInterventionComponent>();
}

UDOSReadableComponent* ADOSPlayerController::FindBestReadable() const
{
	return FindBestComponent<UDOSReadableComponent>();
}

void ADOSPlayerController::OpenDocument(UDOSReadableComponent& Document)
{
	bIsReadingDocument = true;
	Document.MarkRead();

	// 읽는 동안 시간을 멈춘다. 위젯이 CloseDocument()를 부르지 않으면 멈춘 채로 남는다.
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UDOSTimelineSubsystem* Timeline = GameInstance->GetSubsystem<UDOSTimelineSubsystem>())
			{
				Timeline->Pause();
			}
		}
	}

	// 위젯이 꽂혀 있으면 직접 띄운다. 델리게이트도 같이 쏘므로 소리 같은 것을
	// 블루프린트에서 따로 걸 수 있다.
	if (DocumentWidget)
	{
		DocumentWidget->ShowDocument(Document.Title, Document.Body);
	}
	OnDocumentOpened.Broadcast(Document.Title, Document.Body);
}

void ADOSPlayerController::TogglePause(const FInputActionValue& Value)
{
	if (bIsReadingDocument)
	{
		// 문서를 읽는 중이면 ESC는 문서를 닫는다. 일시정지가 겹쳐 뜨지 않는다.
		if (DocumentWidget)
		{
			DocumentWidget->CloseDocument();
		}
		else
		{
			CloseDocument();
		}
		return;
	}

	if (!PauseWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DOS] %s: Pause Widget Class가 비어 있습니다."), *GetName());
		return;
	}

	if (PauseWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		PauseWidget->ClosePause();
	}
	else
	{
		PauseWidget->OpenPause();
	}
}

void ADOSPlayerController::CloseDocument()
{
	if (!bIsReadingDocument)
	{
		return;
	}
	bIsReadingDocument = false;

	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UDOSTimelineSubsystem* Timeline = GameInstance->GetSubsystem<UDOSTimelineSubsystem>())
			{
				Timeline->Play();
			}
		}
	}

	OnDocumentClosed.Broadcast();
	RefreshFocus();
}

void ADOSPlayerController::RefreshFocus()
{
	if (bIsReadingDocument)
	{
		return; // 문서를 펼쳐둔 동안에는 안내를 갱신하지 않는다.
	}

	// 아직 안 읽은 문서가 먼저다. 편지처럼 읽고 나서 밀어야 하는 물건이 있으므로,
	// 읽기가 끝난 뒤에야 밀기 안내로 넘어간다.
	UDOSReadableComponent* Document = FindBestReadable();
	if (Document && (!Document->IsWithinWindow() || Document->HasBeenRead()))
	{
		Document = nullptr;
	}

	UDOSInterventionComponent* Found = FindBestInteractable();

	// 시간창이 지났거나, 1회차이거나, 아직 그 장면을 목격하지 못한 물건은 **안내 자체를
	// 띄우지 않는다.** 눌러도 아무 일이 없으면 플레이어는 버그로 읽고, 1회차에 만질 수 있는
	// 물건이 보이면 관찰에 쏠려야 할 주의가 그리로 샌다.
	if (Found && (!Found->IsWithinWindow() || Found->IsActiveThisRound() || !Found->IsUnlocked()))
	{
		Found = nullptr;
	}

	if (Document == FocusedDocument.Get() && Found == FocusedInteractable.Get())
	{
		return;
	}

	FocusedDocument = Document;
	FocusedInteractable = Found;

	const bool bHasTarget = Document != nullptr || Found != nullptr;
	FText Prompt = FText::GetEmpty();
	if (Document)
	{
		Prompt = Document->PromptText;
	}
	else if (Found)
	{
		Prompt = Found->PromptText;
	}
	// **마지막 상태를 들고 있는다.** HUD 위젯이 컨트롤러보다 늦게 붙으면 이 브로드캐스트를
	// 놓치고, 다음 변화가 올 때까지 안내가 비어 있다. 위젯이 붙을 때 이 값으로 맞춘다.
	bHasInteractTarget = bHasTarget;
	InteractPrompt = Prompt;

	OnInteractPromptChanged.Broadcast(bHasTarget, Prompt);
}

void ADOSPlayerController::Interact(const FInputActionValue& Value)
{
	if (bIsReadingDocument)
	{
		return; // 닫기는 위젯이 CloseDocument()로 처리한다.
	}

	// 문서가 먼저다. 읽고 나면 HasBeenRead()가 true라 다음부터는 밀기로 넘어간다.
	if (UDOSReadableComponent* Document = FocusedDocument.Get())
	{
		OpenDocument(*Document);
		return;
	}

	UDOSInterventionComponent* Target = FocusedInteractable.Get();
	if (!Target)
	{
		// 타이머 사이에 눌렀을 수 있으니 한 번 더 찾아본다.
		Target = FindBestInteractable();
	}
	if (!Target)
	{
		return;
	}

	const EDOSInteractResult Result = Target->Interact();
	OnInteractResult.Broadcast(Result, Target->InterventionId);

	if (Result == EDOSInteractResult::Rejected)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[DOS] 개입 실패: %s (시간창 밖이거나 이미 성립함)"), *Target->InterventionId.ToString());
	}

	RefreshFocus();
}
