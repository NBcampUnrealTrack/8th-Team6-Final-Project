// Attached to any interactable object that can change the outcome of the case (a gas valve,
// a wine bottle, ...). Only usable within its configured time window; a successful use
// registers the intervention with UDOSCaseSubsystem for this round.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DOSInterventionComponent.generated.h"

class UDOSTimelineSubsystem;
class UDOSCaseSubsystem;

// 성공했을 때 한 번 쏜다. 소리·파티클·오브젝트 상태 변화를 블루프린트에서 여기에 건다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDOSOnIntervened);

// 한 번 눌렀을 때 무슨 일이 있었는가. **리듬 노크 때문에 bool로는 모자란다** -
// 여덟 번을 두드려야 성립하므로 "받아들였지만 아직 안 끝났다"가 필요하다.
UENUM(BlueprintType)
enum class EDOSInteractResult : uint8
{
	// 시간창 밖이거나 이미 성립했다. 실패음.
	Rejected,
	// 입력은 받았지만 아직 성립하지 않았다. 노크 한 번 - 두드리는 소리만 난다.
	Acknowledged,
	// 개입이 성립했다. 성공음.
	Succeeded
};

UCLASS(ClassGroup = (DOS), meta = (BlueprintSpawnableComponent))
class DOS_API UDOSInterventionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDOSInterventionComponent();

	// Key this intervention is tracked/matched against in ending rules and schedule overrides.
	//
	// ⚠ **스케줄 override가 쓰는 이름과 글자까지 같아야 한다: t1 / t2 / t3 / t4 / t5found.**
	// interventions.json은 knife/sugar/knock/... 로 부르지만 schedules.json의 override 27개는
	// 전부 t* 를 쓴다. BuildResolvedWaypoints가 문자열로 비교하므로 어긋나면 **아무 일도
	// 일어나지 않고 에러도 안 난다.**  → Docs/Demo_Plan_t2.md
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	FName InterventionId = NAME_None;

	// 화면에 뜨는 동사. 유령이 할 수 있는 것은 미는 것뿐이지만 대상에 따라 읽히는 말이
	// 다르다 - 보일러실 벽은 "두드리기", 식칼은 "밀기".
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	FText PromptText;

	// 이 장면을 **직접 본 뒤에만** 밀 수 있다. 비워두면 처음부터 가능하다.
	// 짝이 되는 것은 FDOSWaypoint::WitnessId.
	//   t2 식칼   ← 세바스찬이 칼을 집어드는 20:10
	//   t4 설탕통 ← 줄리안이 독을 흘려 넣는 19:00
	//   t1 노크   ← 앨리스가 리듬으로 두드리는 19:15
	//   t3 팔찌   ← 빅토리아가 팔찌를 풀어 두는 18:50
	//   t5 편지   ← 플레이어가 각서를 읽는 것 (UDOSReadableComponent 쪽)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	FName RequiredWitnessId = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Intervention")
	FDOSOnIntervened OnIntervened;

	// 1회차인가, 아직 목격하지 못했는가. 둘 중 하나라도 걸리면 밀 수 없다.
	//
	// **1회차 개입 금지**는 Story_Design.md 9행의 확정 규칙이다 - 1회차는 빨리감기(관찰)만
	// 가능하고, 그래서 자동으로 화재 엔딩을 보게 된다.
	// ⚠ 이 제한은 **개입에만** 건다. 문서 읽기와 목격은 1회차부터 된다 - 오히려 그게
	// 1회차의 목적이다(칼 집는 것을 봐야 2회차에 밀 수 있다).
	UFUNCTION(BlueprintPure, Category = "DOS|Intervention")
	bool IsUnlocked() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	float WindowStartSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Intervention")
	float WindowEndSeconds = 1800.f;

	// 플레이어가 한 번 눌렀다. 컨트롤러가 부르는 것은 이쪽이다.
	// 기본 구현은 곧바로 TryIntervene()으로 넘긴다 - 미는 물건은 한 번이면 끝이므로.
	// 리듬 노크처럼 여러 번 눌러야 하는 것은 이 함수를 오버라이드한다.
	UFUNCTION(BlueprintCallable, Category = "DOS|Intervention")
	virtual EDOSInteractResult Interact();

	// Attempts to register the intervention. Fails silently (returns false) if the current
	// timeline time is outside the window or it was already triggered this round.
	UFUNCTION(BlueprintCallable, Category = "DOS|Intervention")
	bool TryIntervene();

	UFUNCTION(BlueprintPure, Category = "DOS|Intervention")
	bool IsWithinWindow() const;

	UFUNCTION(BlueprintPure, Category = "DOS|Intervention")
	bool IsActiveThisRound() const;

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
};
