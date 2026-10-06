// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CustomerAI.generated.h"

UENUM(BlueprintType)
enum class ECustomerState : uint8
{
    PassingBy,          // 1. 스폰 지점(A)에서 퇴장 지점(B)을 향해 그냥 지나가는 중
    WalkingToShelf,     // 2. 가게 앞을 지나다 물건을 발견하고 진열대로 향하는 중
    GrabProduct,        // 3. 상품을 진열대에서 빼서 손에 들기
    WalkingToCash,      // 4. 계산대로 걸어가기
    WaitingForCheckout, // 5. 플레이어의 계산 대기하기
    LeaveShop           // 6. 볼일이 끝나서 퇴장 지점(B)으로 나가는 중
};

UCLASS()
class CH6_SHOP_SIMULATOR_API ACustomerAI : public ACharacter
{
    GENERATED_BODY()

public:
    // Sets default values for this character's properties
    ACustomerAI();

    // 현재 손님의 행동 상태
    ECustomerState CurrentState = ECustomerState::PassingBy;

    // 이 손님이 찜해둔 진열대 목표물
    class ADisplayBase* TargetDisplay = nullptr;

    // ★ 계산대로 걸어갈 때 쓸 목표 계산대. TargetDisplay랑 똑같은 이유로,
    // 매 프레임 월드 전체를 다시 뒤지지 않고 한 번 찾은 걸 기억해뒀다가 재사용함.
    class ACounterBase* TargetCounter = nullptr;

    // ★ 이 손님이 향하는 퇴장 지점. 스폰될 때 한 번 뽑아서 계속 사용함.
    // (매번 다시 뽑으면 랜덤 위치가 흔들려서 손님이 갈팡질팡함)
    FVector ExitLocation = FVector::ZeroVector;

    // 가게 감지 구역에서 이미 판단을 마쳤는지. 한 번 지나친 손님이
    // 계속 재판정되는 것을 막기 위함.
    bool bHasCheckedShop = false;

    // ★ 마지막으로 MoveToLocation을 호출했을 때의 목표 좌표. 진열대가 이동했는지
    // 확인할 때 이 값이랑 비교해서, 달라졌으면 목적지를 다시 갱신함 (진열대가
    // 걸어가는 도중에 재배치되어도 손님이 새 위치로 따라가게 하기 위함).
    FVector LastMoveTargetLocation = FVector::ZeroVector;

    // AI에게 어디로 걸어가라고 명령하는 함수
    void MoveToLocation(FVector GoalLocation);

    void GrabAndGoToCash();

    // 가게 감지 구역에 들어왔을 때, 진열된 상품이 있으면 그쪽으로 방향을 틀게 함.
    // 없으면 아무것도 안 하고 계속 지나감.
    void TryEnterShop();

    // 볼일이 끝났거나 살 게 없을 때 퇴장 지점으로 향하게 함
    void HeadToExit();

    /** 🔥 [모듈화 함수]: 지정된 초(InDelay) 이후에 나 자신(손님)을 월드에서 영구 파괴하는 시한폭탄 가동 기능 */
    void StartDestroyTimer(float InDelay);
protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // 타이머를 관리해 줄 전용 핸들 가방 선언
    FTimerHandle SelfDestroyTimerHandle;

    // 월드에서 물건이 차 있는 진열대를 찾아 반환하는 안전한 함수
    class ADisplayBase* FindValidDisplayInWorld();
public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};