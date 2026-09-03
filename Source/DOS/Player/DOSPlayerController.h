// 플레이어 컨트롤러 - 입력의 정본. 매핑 컨텍스트 등록과 액션 바인딩을 전부 여기서 한다.
//
// **폰이 아니라 컨트롤러에 두는 이유**: 폰은 빙의 순서에 휘둘린다. 게임모드가 스폰한 폰은
// SetupPlayerInputComponent가 BeginPlay보다 먼저 불리고, 레벨에 직접 배치한 폰은 그 반대다.
// 컨트롤러는 처음부터 LocalPlayer를 갖고 있어 BeginPlay 시점이면 언제나 준비가 끝나 있다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Case/DOSInterventionComponent.h"
#include "DOSPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UDOSInterventionComponent;
class UDOSReadableComponent;
class UDOSDocumentWidget;
class UDOSPauseWidget;
class UDOSHUDWidget;
struct FInputActionValue;

// 문서를 펼쳤다. 전체 화면 텍스트 위젯이 여기에 붙는다. **읽는 동안 타임라인은 멈춘다** -
// 편지를 읽느라 다른 방의 장면을 놓치면 가혹하다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDOSOnDocumentOpened, const FText&, Title, const FText&, Body);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDOSOnDocumentClosed);

// 지금 눈앞에 만질 수 있는 것이 있는지, 있으면 뭐라고 표시할지. 화면 안내 위젯이 여기에 붙는다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDOSOnInteractPromptChanged, bool, bHasTarget, const FText&, Prompt);

// 실제로 눌렀을 때. 성공/실패/진행중 소리를 여기에 건다.
// **Acknowledged가 따로 있는 이유는 리듬 노크다** - 여덟 번 두드리는 동안 매번 실패음이
// 나면 안 된다. 그때는 두드리는 소리만 난다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDOSOnInteractResult, EDOSInteractResult, Result, FName, InterventionId);

UCLASS()
class DOS_API ADOSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// 에디터에서 꽂는다. /Game/DoS/Inputs/ 아래 셋이 정본이다.
	//   MappingContext -> IMC_Player   MoveAction -> IA_Move   LookAction -> IA_Look
	// IMC_Player가 물고 있는 액션과 아래 두 칸이 **같은 에셋**이어야 한다. 이름만 같은
	// 다른 에셋을 꽂으면 키는 들어오는데 아무것도 안 움직인다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputAction> InteractAction;

	// 마우스 휠. 1회차에도 쓸 수 있어야 한다 - 그때는 빨리감기가 유일한 조작이다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputAction> SpeedAction;

	// 일시정지. ESC를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input")
	TObjectPtr<UInputAction> PauseAction;

	// --- 위젯 ----------------------------------------------------------------
	// 둘 다 BeginPlay에서 한 번 만들어 뷰포트에 붙여두고 숨긴다. 열 때마다 새로 만들면
	// 문서를 열 때마다 위젯 생성 비용이 들고, 애니메이션 상태도 매번 초기화된다.

	UPROPERTY(EditDefaultsOnly, Category = "DOS|UI")
	TSubclassOf<UDOSDocumentWidget> DocumentWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "DOS|UI")
	TSubclassOf<UDOSPauseWidget> PauseWidgetClass;

	// 상시 화면. 상호작용 키 안내가 여기 뜬다. 비워두면 안내가 아예 없다 -
	// 무언극이라 조작 안내는 화면과 소리뿐이므로 데모에 반드시 필요하다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|UI")
	TSubclassOf<UDOSHUDWidget> HUDWidgetClass;

	UFUNCTION(BlueprintPure, Category = "DOS|UI")
	UDOSDocumentWidget* GetDocumentWidget() const { return DocumentWidget; }

	UFUNCTION(BlueprintPure, Category = "DOS|UI")
	UDOSPauseWidget* GetPauseWidget() const { return PauseWidget; }

	UFUNCTION(BlueprintPure, Category = "DOS|UI")
	UDOSHUDWidget* GetHUDWidget() const { return HUDWidget; }

	// 지금 안내가 떠 있어야 하는지, 그리고 그 문구. HUD가 늦게 붙었을 때 현재 상태로
	// 맞추는 용도다 - 이벤트를 놓친 위젯이 빈 안내를 들고 있으면 안 된다.
	UFUNCTION(BlueprintPure, Category = "DOS|Interaction")
	bool HasInteractTarget() const { return bHasInteractTarget; }

	UFUNCTION(BlueprintPure, Category = "DOS|Interaction")
	FText GetInteractPrompt() const { return InteractPrompt; }

	// 화면 좌상단에 시각·배속·회차를 찍는다. 정식 HUD가 나오기 전까지 쓰는 임시 표시다 —
	// 일정표 좌표를 채우거나 촬영할 때 "지금 몇 시인지"를 봐야 한다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Debug")
	bool bShowTimelineDebug = true;

	// 휠 한 칸에 한 단계씩 오간다. 연속값이 아니라 프리셋인 이유는 정확히 ×1로 돌아올 수
	// 있어야 하기 때문이다.
	// ×0.5는 느리게 감기다. 무언극이라 자세·시선이 정보인데 오인 살해(21:05)나 주방
	// 대치(22:20) 같은 장면은 ×1로도 순식간에 지나간다.
	// 난이도는 안 깨진다 - 전역 시간 배율이라 걸음도 같이 느려져서, 15초 창 안에
	// 보일러실에 닿는 난이도는 그대로다. 실제 반응 시간만 늘어난다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Timeline")
	TArray<float> SpeedSteps = { 0.5f, 1.f, 2.f, 4.f, 8.f };

	UPROPERTY(EditDefaultsOnly, Category = "DOS|Input", meta = (ClampMin = "0.01"))
	float LookSensitivity = 0.6f;

	// --- 상호작용 -----------------------------------------------------------

	// 폰에서 이 거리 안에 있는 개입 오브젝트만 잡는다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Interaction", meta = (ClampMin = "10.0"))
	float InteractRange = 220.f;

	// 시선에서 이 각도 안에 있어야 한다. 등 뒤의 물건이 잡히면 안내가 엉뚱하게 뜬다.
	UPROPERTY(EditDefaultsOnly, Category = "DOS|Interaction", meta = (ClampMin = "5.0", ClampMax = "180.0"))
	float InteractHalfAngleDegrees = 75.f;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Interaction")
	FDOSOnInteractPromptChanged OnInteractPromptChanged;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Interaction")
	FDOSOnInteractResult OnInteractResult;

	// 지금 잡히는 대상. 없으면 null.
	UFUNCTION(BlueprintPure, Category = "DOS|Interaction")
	UDOSInterventionComponent* GetFocusedInteractable() const { return FocusedInteractable.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "DOS|Document")
	FDOSOnDocumentOpened OnDocumentOpened;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Document")
	FDOSOnDocumentClosed OnDocumentClosed;

	// 문서 화면을 닫는다. 전체 화면 위젯의 닫기 버튼이 이걸 부른다 - 부르지 않으면
	// 타임라인이 멈춘 채로 남는다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Document")
	void CloseDocument();

	UFUNCTION(BlueprintPure, Category = "DOS|Document")
	bool IsReadingDocument() const { return bIsReadingDocument; }

protected:
	// 매핑 컨텍스트 등록. LocalPlayer가 이미 있으므로 여기가 확실하다.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	// 액션 바인딩. 컨트롤러의 InputComponent는 폰 빙의와 무관하게 만들어진다.
	virtual void SetupInputComponent() override;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ChangeSpeed(const FInputActionValue& Value);
	void TogglePause(const FInputActionValue& Value);

	// 폰 주변에서 시선에 가장 잘 맞는 것을 고른다. 없으면 null.
	// 문서와 개입 오브젝트를 같은 방식으로 훑는다 - 템플릿으로 묶어 규칙이 갈라지지 않게 한다.
	template <typename TComponent>
	TComponent* FindBestComponent() const;

	UDOSInterventionComponent* FindBestInteractable() const;
	UDOSReadableComponent* FindBestReadable() const;

	void OpenDocument(UDOSReadableComponent& Document);

	// 안내 문구를 갱신한다. 매 프레임이 아니라 타이머로 돈다 - 이 판정은 그렇게 자주
	// 정확할 필요가 없고, 겹침 질의를 매 틱 돌릴 이유도 없다.
	void RefreshFocus();

	TWeakObjectPtr<UDOSInterventionComponent> FocusedInteractable;
	TWeakObjectPtr<UDOSReadableComponent> FocusedDocument;
	FTimerHandle FocusTimer;
	bool bIsReadingDocument = false;

	UPROPERTY(Transient)
	TObjectPtr<UDOSDocumentWidget> DocumentWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDOSPauseWidget> PauseWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDOSHUDWidget> HUDWidget;

	// RefreshFocus가 마지막으로 쏜 값. HasInteractTarget/GetInteractPrompt가 읽는다.
	bool bHasInteractTarget = false;
	FText InteractPrompt;
};
