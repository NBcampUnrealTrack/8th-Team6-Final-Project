// 스케줄대로 움직이는 일곱 명 중 하나. UDOSCharacterScheduleComponent가 "지금 어디서 무엇을
// 하고 있어야 하는가"를 내놓으면, 이 액터가 **그 지점까지 실제로 걸어간다.**
//
// 왜 컴포넌트에 맡기지 않는가: 컴포넌트의 SampleAtTime은 웨이포인트 사이를 선형 보간한다.
// 그대로 트랜스폼에 찍으면 벽을 통과해 미끄러진다. 웹 프로토타입은 타일 경로탐색으로 실제
// 보행 시간을 따졌고("걸어서 다음 지시 전에 닿는가" 검사가 그것이다), 그 전제를 유지하려면
// 여기서도 NavMesh 위를 걸어야 한다.
//
// NavMesh가 아직 없는 레벨(블록아웃 전 Showcase 등)에서는 걷기가 실패하므로, 그때는
// 보간 배치로 되돌아간다 - 아무것도 안 움직이는 것보다는 낫고, "NavMesh를 깔아야 한다"는
// 신호로도 쓰인다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/PoseSnapshot.h"
// FAIRequestID / EPathFollowingResult - HandleMoveCompleted 서명에 필요하다.
#include "Navigation/PathFollowingComponent.h"
#include "DOSTimelineTypes.h"
#include "DOSScheduledCharacter.generated.h"

class UDOSCharacterScheduleComponent;
class UDOSCaseSubsystem;
class UDOSCharacterAnimSet;
class UAnimMontage;

// 시신은 네 상태를 오간다. 물리를 전부 켜거나 전부 끄면 둘 중 하나가 깨진다 -
// 래그돌인 채로는 위치를 못 옮기고(스케줄·되감기가 죽는다), 물리를 그냥 끄면 누운
// 애니메이션이 없어 서 있는 자세로 튕겨 돌아온다.  → Docs/Demo_Plan_t2.md
UENUM(BlueprintType)
enum class EDOSCorpseState : uint8
{
	// 살아 있다. 스케줄대로 걸어다닌다.
	Alive,
	// 방금 죽어 전신이 물리에 넘어간 상태. 쓰러지는 그림이 여기서 나온다.
	Ragdoll,
	// 쓰러진 자세를 스냅샷으로 붙잡고 물리를 껐다. 평범한 액터라 위치를 옮길 수 있다.
	Frozen,
	// 잡힌 뼈 하나만 kinematic, 나머지는 시뮬레이션 - 끌 때 축 늘어진다. (미구현)
	Dragged
};

UCLASS()
class DOS_API ADOSScheduledCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADOSScheduledCharacter();

	// 웹 프로토타입 쪽 id와 같은 값을 쓴다: eleanor / julian / sebastian / alice / victoria /
	// rose / desmond. 스케줄 에셋을 자동으로 물리거나 나레이션에서 이름을 찾을 때 쓴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character")
	FName CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DOS|Character")
	TObjectPtr<UDOSCharacterScheduleComponent> Schedule;

	// 꺼두면 스케줄이 트랜스폼을 곧바로 찍는다(종전 동작). 켜면 NavMesh 위를 걷는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character")
	bool bWalkWithNavigation = true;

	// 목표 지점에서 이만큼 안이면 도착으로 본다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "1.0"))
	float AcceptanceRadius = 60.f;

	// 목표 지점을 NavMesh로 끌어당길 때 찾아보는 범위. 웨이포인트 사이를 직선 보간하므로
	// 목표가 벽 속이나 층 사이 공중에 있을 수 있다 - 층 하나를 덮을 만큼은 넉넉해야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character")
	FVector NavProjectionExtent = FVector(600.f, 600.f, 600.f);

	// **걸음 속도를 남은 시간에 맞춘다.** 웹 프로토타입은 「타일 하나 = 타임라인 1초」로
	// 설계되어 항상 제때 닿았는데, 3D 저택은 거리가 다르므로 그 전제가 깨진다.
	// 남은 시간과 남은 거리로 속도를 정하면 3D 치수와 무관하게 스케줄이 성립한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character")
	bool bAdaptWalkSpeedToSchedule = true;

	// 맞춘 속도를 이 범위로 묶는다. 너무 느리면 기어가는 것으로 보이고, 너무 빠르면
	// 미끄러지는 것으로 보인다. 상한을 넘어야 닿는 거리면 경고를 남긴다 -
	// 그건 방이 너무 멀다는 뜻이므로 배치를 고쳐야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "10.0"))
	float MinAdaptedWalkSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "10.0"))
	float MaxAdaptedWalkSpeed = 420.f;

	// 웨이포인트 좌표가 NavMesh에서 이보다 멀면 **이동하지 않는다.** 억지로 끌어당겨
	// 걸어가면 엉뚱한 자리에 도착해 멈춰 선 것처럼 보이므로, 좌표를 고치라고 경고만 남긴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "10.0"))
	float MaxNavSnapDistance = 150.f;

	// **걸어 올라갈 수 있는 최대 경사(도).**
	//
	// 저택 계단(SM_StairsCarpet)의 콜리전은 계단 모양이 아니라 매끄러운 **52° 경사면**이다.
	// 엔진 기본값 45°나 이전 값 50°로는 「걸을 수 없는 면」이 되어, 인물이 계단 중간에서
	// 밀기만 하다가 경로 추종이 Blocked로 끝난다. 2도 차이라 진입 각도에 따라 올라가기도
	// 해서 간헐적으로 보였다.
	//
	// 계단 콜리전을 더 완만하게 만들면 낮춰도 된다. 값을 올릴 때는 **의도치 않은 경사면
	// (지붕, 화단 둔덕)까지 걸어 올라가게 되지 않는지** 봐야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "20.0", ClampMax = "80.0"))
	float WalkableFloorAngleDegrees = 55.f;

	// 경로를 못 찾았을 때 다시 시도하는 주기(초). 웨이포인트가 바뀔 때까지 기다리면
	// 한 번 실패한 자리에서 영영 멈춰 선다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Character", meta = (ClampMin = "0.2"))
	float WalkRetryInterval = 1.f;

	// 화면에 목표·거리·이동상태·속도를 찍는다. 원인 추적용 - 촬영 전에 끈다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Debug")
	bool bShowWalkDebug = true;

	// 지금 재생 중인 상태. 나레이션·원온원·열쇠구멍이 이걸 읽는다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FDOSCharacterState CurrentState;

	// 목표 지점까지 아직 걸어가는 중인가. "이동하고 있다"와 "그 방에 자리를 잡았다"를
	// 구분해야 하는 곳이 있다(원온원은 자리를 잡은 뒤에만 성립한다).
	UFUNCTION(BlueprintPure, Category = "DOS|Character")
	bool IsWalking() const;

	// --- 시신 ---------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Corpse")
	EDOSCorpseState CorpseState = EDOSCorpseState::Alive;

	// 쓰러진 자세. AnimBP의 **Pose Snapshot 노드**가 이걸 읽어 재생한다
	// (bHasFrozenPose가 true일 때만).
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Corpse")
	FPoseSnapshot FrozenPose;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Corpse")
	bool bHasFrozenPose = false;

	// 무너진 뒤 자세가 안정될 때까지 기다리는 시간. **실제 초**다 - 타임라인 배속을 올려도
	// 물리는 실시간으로 돌기 때문이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Corpse", meta = (ClampMin = "0.1"))
	float SettleSeconds = 2.5f;

	// --- 애니메이션 ---------------------------------------------------------

	// 이 인물만의 동작 표. 공용과 **달라야 하는 것만** 넣는다.
	// 자식 BP(BP_Sebastian 등)의 Class Defaults에서 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Anim")
	TObjectPtr<UDOSCharacterAnimSet> AnimSet;

	// 일곱 명이 공유하는 기본 동작 표. **부모 BP에 한 번 꽂아두면 자식들이 상속받는다.**
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Anim")
	TObjectPtr<UDOSCharacterAnimSet> SharedAnimSet;

	// 지금 박자에 재생할 몽타주. 세 단으로 찾는다:
	//   ① 웨이포인트의 ActionMontage → ② 인물별 AnimSet → ③ 공용 SharedAnimSet
	// 없으면 nullptr. 블루프린트의 OnCharacterStateApplied에서 이걸 불러 Play Montage 한다.
	UFUNCTION(BlueprintPure, Category = "DOS|Anim")
	UAnimMontage* ResolveActionMontage() const;

	// --- 목격 -------------------------------------------------------------

	// 이 거리 안이어야 목격으로 친다. 방 하나를 가로지르는 정도.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Witness", meta = (ClampMin = "100.0"))
	float WitnessRange = 1400.f;

	// 시선에서 이 각도 안이어야 한다. 화면 밖에서 벌어진 일을 봤다고 할 수는 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Witness", meta = (ClampMin = "5.0", ClampMax = "180.0"))
	float WitnessHalfAngleDegrees = 45.f;

	// 목격 판정 주기(초). 매 틱 라인 트레이스를 쏠 이유가 없다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Witness", meta = (ClampMin = "0.05"))
	float WitnessCheckInterval = 0.2f;

	// 데스먼드가 손목을 잡고 끌기 시작할 때. 잡힌 뼈만 kinematic으로 돌리고 나머지는
	// 시뮬레이션에 두면 팔·머리·다리가 중력에 매달려 끌린다. (미구현 - 발표 뒤)
	UFUNCTION(BlueprintCallable, Category = "DOS|Corpse")
	void BeginDragged(FName HeldBoneName);

	// 내려놓으면 다시 안정될 때까지 시뮬레이션했다가 Frozen으로. (미구현 - 발표 뒤)
	UFUNCTION(BlueprintCallable, Category = "DOS|Corpse")
	void EndDragged();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleStateApplied(const FDOSCharacterState& State);

	// 이번 회차의 개입이 바뀌면 일정표를 다시 짠다. 이 배선이 없으면 트리거를 눌러도
	// 아무 일도 일어나지 않는다 - 오래 비어 있던 자리다.
	UFUNCTION()
	void HandleInterventionsChanged();

	// 지금 재생 중인 박자가 목격 대상이면, 플레이어에게 실제로 보이는지 재서 지식을 준다.
	// 같은 방에 있는 것만으로는 부족하다 - **보고 있어야** 한다.
	void CheckWitness();
	bool IsVisibleToPlayer() const;

	// 남은 시간 안에 목표까지 닿도록 MaxWalkSpeed를 맞춘다.
	void AdaptWalkSpeed();

	void EnterRagdoll();
	// 쓰러진 자세를 스냅샷으로 뜨고 물리를 끈다. Ragdoll -> Frozen.
	void FreezePose();
	// 되감기로 죽기 전으로 돌아갔을 때. Frozen/Ragdoll -> Alive.
	void Revive();

	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
	FTimerHandle SettleTimer;
	// 되살아날 때 메시를 제자리에 돌려놓기 위해 BeginPlay에서 받아둔다. 래그돌은 메시의
	// 트랜스폼을 물리에 넘기므로, 물리를 끄면 어디에 있어야 하는지 알 방법이 없어진다.
	FTransform InitialMeshRelativeTransform;
	float WitnessCheckAccumulator = 0.f;
	float WalkRetryAccumulator = 0.f;
	// 마지막 경로 요청 결과. 화면 디버그에 그대로 찍는다.
	FString LastWalkResult = TEXT("-");

	// **경로 추종이 어떻게 끝났는지.** 계단 중간에 멈추는 원인은 둘 중 하나이고
	// 이 값이 그걸 구분한다:
	//   Success  → 도착했다고 판단했다. 목표(투영된 웨이포인트)가 계단에 있는 것이다.
	//   Blocked  → 밀고 있는데 지오메트리에 막혔다. MaxStepHeight/콜리전 문제다.
	//   Aborted  → 다른 이동 요청이 덮어썼다. 내 재시도 로직이 스스로 끊은 것이다.
	//   OffPath  → 경로에서 밀려났다.
	// 부분경로(IsPartial)면 목표까지 길이 없어 도중까지만 간 것이다.
	FString LastMoveResult = TEXT("-");

	// AAIController::ReceiveMoveCompleted에 붙는다.
	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	// NavMesh로 걸어가게 시킨다. 경로를 못 찾으면 false - 호출한 쪽이 보간 배치로 되돌린다.
	bool RequestWalkTo(const FVector& Target);

	FVector PendingTarget = FVector::ZeroVector;
	bool bHasTarget = false;
	// NavMesh가 없어 걷기를 포기한 상태. 매 틱 경로 요청을 반복하지 않기 위한 래치다.
	bool bNavigationUnavailable = false;
};
