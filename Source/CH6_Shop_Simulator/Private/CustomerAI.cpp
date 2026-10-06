// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomerAI.h"
#include "AIController.h" // AI 기능 필수 인클루드
#include "DisplayBase.h"
#include "CounterBase.h"
#include "CustomerZone.h"
#include "Blueprint/AIBlueprintHelperLibrary.h" 
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "EngineUtils.h"

// Sets default values
ACustomerAI::ACustomerAI()
{
    // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    // 1. AI 컨트롤러의 요(Yaw) 회전값에 몸을 맞추지 않도록 합니다.
    bUseControllerRotationYaw = false;

    // 2. AIController가 가진 회전값에 캐릭터가 영향받지 않도록 설정 (★핵심)
    if (AController* AICont = GetController())
    {
        AICont->SetControlRotation(FRotator::ZeroRotator);
        // 또는 AIController로 캐스팅이 가능하다면 아래 옵션을 명시적으로 끕니다.
        // AAIController* ExactAICont = Cast<AAIController>(AICont);
        // if(ExactAICont) ExactAICont->bUpdateControlRotation = false;
    }

    // 3. 이동 컴포넌트 설정
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    }

    // AI 컨트롤러가 자동으로 이 캐릭터를 조종하도록 설정
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACustomerAI::BeginPlay()
{
    Super::BeginPlay();

    CurrentState = ECustomerState::PassingBy;
    TargetDisplay = nullptr;
    bHasCheckedShop = false;

    // ★ 퇴장 지점(B구역)을 레벨 마커에서 찾아 한 번만 뽑아둠. 매번 다시 뽑으면
    // 랜덤 위치가 흔들려서 손님이 갈팡질팡하므로 여기서 확정하고 계속 사용함.
    if (ACustomerZone* ExitZone = ACustomerZone::FindZone(this, ECustomerZoneType::Exit))
    {
        ExitLocation = ExitZone->GetRandomPoint();
    }
    else
    {
        // 마커를 안 놓았으면 제자리에 서있게 됨 (레벨 세팅 누락 알림)
        ExitLocation = GetActorLocation();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
            TEXT("⚠ CustomerAI: 레벨에 Exit 타입 CustomerZone이 없습니다!"));
    }

    // 일단 지나가는 손님으로 출발. 가게 앞을 지나다 물건이 있으면 그때 들어감.
    MoveToLocation(ExitLocation);
}

// CustomerAI.cpp 의 Tick 이나 타이머 시스템 예시 구상
void ACustomerAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // [상태 1]: 퇴장 지점을 향해 그냥 지나가는 중. 가게 감지 구역에 들어오면
    // 그때 한 번만 "살 게 있나?" 판단하고, 없으면 계속 지나감.
    if (CurrentState == ECustomerState::PassingBy)
    {
        if (!bHasCheckedShop)
        {
            if (ACustomerZone* ShopZone = ACustomerZone::FindZone(this, ECustomerZoneType::ShopEntrance))
            {
                if (ShopZone->IsLocationInside(GetActorLocation()))
                {
                    bHasCheckedShop = true; // 한 번 판단했으면 다시 안 봄
                    TryEnterShop();
                }
            }
        }

        // 퇴장 지점에 도착했으면 소멸
        if (FVector::Dist(GetActorLocation(), ExitLocation) <= 150.f)
        {
            Destroy();
        }
    }

    // [상태 2]: 매장 안 진열대 정면 발바닥 슬롯으로 걸어가는 중인 경우
    else if (CurrentState == ECustomerState::WalkingToShelf && TargetDisplay && TargetDisplay->CustomerWaitSlot)
    {
        const FVector CurrentWaitSlotLocation = TargetDisplay->CustomerWaitSlot->GetComponentLocation();

        // ★ [진열대 재배치 대응]: 걸어가는 도중에 진열대가 다른 곳으로 옮겨졌는지
        // 확인. 마지막으로 명령 내렸던 목적지랑 지금 진열대 위치가 많이 달라졌으면
        // (=플레이어가 그 사이에 재배치함), 새 위치로 다시 걸어가도록 명령을 새로 냄.
        // (안 그러면 손님이 예전 좌표에 도착해서 더 이상 안 움직이는데, 도착 판정은
        // 항상 "지금 진열대 위치" 기준이라서 영원히 도착 판정이 안 나는 버그가 있었음)
        constexpr float ShelfMovedThreshold = 50.f;
        if (FVector::DistSquared(CurrentWaitSlotLocation, LastMoveTargetLocation) > FMath::Square(ShelfMovedThreshold))
        {
            MoveToLocation(CurrentWaitSlotLocation);
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta, TEXT("[재추적] 진열대가 이동해서 목적지를 다시 갱신합니다."));
        }

        float Distance = FVector::Dist(GetActorLocation(), CurrentWaitSlotLocation);

        // 진열대 앞 1.2미터 이내로 도달했다면?
        if (Distance <= 120.f)
        {
            // 걸어오는 도중에 앞 손님이 물건을 채가서 빈 진열대가 되었는지 재확인
            if (TargetDisplay->IsSlotEmpty())
            {
                TargetDisplay = nullptr;
                HeadToExit();
            }
            else
            {
                // 물건이 남아있으면 곧바로 집어서 계산대로
                CurrentState = ECustomerState::GrabProduct;
                GrabAndGoToCash();
            }
        }
    }

    // [상태 3]: 물건을 쥐고 계산대 줄서기 자리를 향해 걸어가는 중인 경우!
    else if (CurrentState == ECustomerState::WalkingToCash && TargetCounter && TargetCounter->CustomerWaitSlot)
    {
        // 내가 대기열 배열 중 몇 번째 서 있는지 번호를 찾습니다.
        int32 MyLineIndex = TargetCounter->CustomerQueue.Find(this);

        if (MyLineIndex != INDEX_NONE)
        {
            // 내가 진짜 서야 할 줄서기 목표 세계 좌표를 계산합니다.
            FVector MyTargetLineLocation = TargetCounter->CustomerWaitSlot->GetComponentLocation() + (TargetCounter->GetActorForwardVector() * (MyLineIndex * 70.f));

            // 내 발바닥과 목표 대기 줄 사이의 거리를 계산합니다.
            float DistanceToLine = FVector::Dist(GetActorLocation(), MyTargetLineLocation);

            // 줄서기 지정 칸 앞 1미터 이내로 드디어 완전히 안착했다면?
            if (DistanceToLine <= 100.f)
            {
                // ★ [핵심 픽스]: 이동 상태를 끄고, 새로 만든 "계산 대기 중" 스위치를 쾅 켭니다!
                CurrentState = ECustomerState::WaitingForCheckout;

                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
                    FString::Printf(TEXT("[계산 대기] 손님이 %d번 대기선 자리에 안착하여 계산 대기중!"), MyLineIndex + 1));
            }
        }
    }

    // [상태 4]: 볼일이 끝나 퇴장 지점으로 나가는 중. 도착하면 소멸.
    else if (CurrentState == ECustomerState::LeaveShop)
    {
        if (FVector::Dist(GetActorLocation(), ExitLocation) <= 150.f)
        {
            Destroy();
        }
    }
}

// 1. 진열대를 찾는 독립된 안전한 비서 함수 구현
ADisplayBase* ACustomerAI::FindValidDisplayInWorld()
{
    for (TActorIterator<ADisplayBase> It(GetWorld()); It; ++It)
    {
        ADisplayBase* SimpleDisplay = *It;
        if (SimpleDisplay)
        {
            if (!SimpleDisplay->IsSlotEmpty())
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("[탐색 성공] 상품이 있는 진열대 발견!"));
                return SimpleDisplay; // ★ 찾자마자 주소를 들고 함수를 통째로 안전하게 즉시 탈출!
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("[탐색 실패] 비어있는 진열대 패스"));
            }
        }
    }
    return nullptr; // 아무것도 못 찾았으면 깔끔하게 nullptr 반환
}

void ACustomerAI::MoveToLocation(FVector GoalLocation)
{
    // ★ 이 목적지를 기억해둬서, 나중에 목표물(진열대 등)이 이동했는지 비교할 때 사용
    LastMoveTargetLocation = GoalLocation;

    // ★ [우회 픽스]: 복잡한 AIController 검증을 생략하고, 
       // 엔진의 이동 도우미 라이브러리를 통해 목적지 좌표로 무조건 강제 직진 걷기를 지시합니다!
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), GoalLocation);

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("[이동] SimpleMoveToLocation 강제 이동!"));
}


// 고민이 끝나면 진열대에서 해당 장비를 손에 쥐고 계산대 줄서기 작동
void ACustomerAI::GrabAndGoToCash()
{
    if (TargetDisplay && !TargetDisplay->IsSlotEmpty())
    {
        AActor* Product = TargetDisplay->OccupiedProduct;
        // 부모인 AProductBase로 안전하게 캐스팅하여 가져옵니다.
        AProductBase* BaseProduct = Cast<AProductBase>(Product);
        if (BaseProduct)
        {
            // 1. 진열대 장부를 깨끗하게 비워서 다른 상자가 올 수 있게 공실 처리합니다.
            TargetDisplay->OccupiedProduct = nullptr;
            BaseProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

            // 🔥 [소유권 잠금 장치 가동 🔒]:
            // 상자에게 "너는 이제 손님 전용 물건이다!"라고 스위치를 쾅 켜줍니다! ✅
            BaseProduct->bIsOwnedByCustomer = true;

            // 2. 진열대 가구와의 자식 관계를 해제합니다.
            Product->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

            // 3. ★ [마네킹 오른손 쥐기]: 임시 캡슐이 아닌, 캐릭터의 실제 메쉬 오른손 소켓("SOC_hand_r")에 결착! 🔗
            // (이전 답변에서 설정한 에셋팩 마네킹 오른손 뼈에 소켓을 만드셨다면 찰떡같이 쥐어집니다)
            Product->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("SOC_hand_r"));

            // 4. 상태를 계산대 걷기 상태로 전환!
            CurrentState = ECustomerState::WalkingToCash;
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("[그랩] 손님이 상품을 오른손에 획득! 계산대로 향합니다."));

            // 5. ★ [대기열 줄서기 시스템 연동]: 월드에서 계산대 가구를 스스로 찾습니다.
            // (TargetCounter는 이제 멤버 변수라, 여기서 한 번만 찾아두면 Tick에서
            // 매번 다시 안 찾고 이 값을 그대로 재사용할 수 있음)
            for (TActorIterator<ACounterBase> It(GetWorld()); It; ++It)
            {
                TargetCounter = *It;
                if (TargetCounter) break; // 월드에 배치된 계산대 가구 첫 번째 녀석 확보!
            }

            // 6. 계산대를 발견했다면 정식으로 대기열 번호를 부여받고 걷기 시작합니다! 🚶‍♂️
            if (TargetCounter && TargetCounter->CustomerWaitSlot)
            {
                // 계산대 장부에 내 이름을 올리고 "내가 서야 할 줄 번호(0번, 1번...)"를 받아옵니다.
                int32 MyLineNumber = TargetCounter->JoinQueue(this);

                // 줄 번호에 따라 내 최종 목적지 좌표를 뒤쪽으로 70cm씩 밀어서 자동 분산 계산합니다! 🗺️
                FVector QueueWaitLocation = TargetCounter->CustomerWaitSlot->GetComponentLocation() + (TargetCounter->GetActorForwardVector() * (MyLineNumber * 70.f));

                // 내 배정 대기선 자리로 강제 직진 걷기 지시!
                MoveToLocation(QueueWaitLocation);

                GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("[계산 대기] 계산대 줄서기 완료! 나는 [ %d ] 번째 대기자입니다."), MyLineNumber + 1));
            }
            else
            {
                // 만약 깜빡하고 계산대 가구를 매장에 안 깔아두었다면 안전빵으로 제자리 정지
                MoveToLocation(GetActorLocation());
                GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("[에러] 매장에 계산대(CounterBase) 가구가 배치되어 있지 않습니다!"));
            }
        }
    }
    else
    {
        // 도착했는데 물건이 사라졌으면(다른 손님이나 플레이어가 가져감) 그냥 나감
        HeadToExit();
    }
}

// 가게 감지 구역에 들어왔을 때 호출됨. 진열된 상품이 있으면 그쪽으로 방향을 틀고,
// 없으면 아무것도 안 하고 그냥 지나가게 둠.
void ACustomerAI::TryEnterShop()
{
    TargetDisplay = FindValidDisplayInWorld();

    if (!TargetDisplay || !TargetDisplay->CustomerWaitSlot)
    {
        // 살 게 없으면 방향을 틀지 않고 원래 가던 길(퇴장 지점)로 계속 감
        TargetDisplay = nullptr;
        return;
    }

    CurrentState = ECustomerState::WalkingToShelf;
    MoveToLocation(TargetDisplay->CustomerWaitSlot->GetComponentLocation());
}

// 볼일이 끝났거나 살 게 없어졌을 때 퇴장 지점으로 향하게 함
void ACustomerAI::HeadToExit()
{
    CurrentState = ECustomerState::LeaveShop;
    TargetDisplay = nullptr;
    MoveToLocation(ExitLocation);

    // 도착 판정이 어떤 이유로든 안 나는 경우를 대비한 안전장치
    StartDestroyTimer(30.0f);
}

void ACustomerAI::StartDestroyTimer(float InDelay) // 5초 후에 손님을 없에는 함수
{
    // 안전장치: 혹시 이미 예약된 타이머가 돌고 있다면 초기화 후 재예약합니다.
    GetWorldTimerManager().ClearTimer(SelfDestroyTimerHandle);

    // ★ [모듈화 핵심]: 전달받은 시간(InDelay) 뒤에 나 자신(this)의 Destroy 함수를 예약 실행합니다!
    // ★ [람다 우회 픽스]: 컴파일러의 엄격한 함수 포인터 검사를 우회하는 정석 람다 문법입니다!
    GetWorldTimerManager().SetTimer(
        SelfDestroyTimerHandle,
        [this]()
        {
            // 지정된 시간이 되었을 때 안전하게 나 자신을 파괴합니다.
            if (IsValid(this))
            {
                TArray<AActor*> AttachedActors;
                this->GetAttachedActors(AttachedActors);

                for (AActor* AttachedActor : AttachedActors)
                {
                    if (IsValid(AttachedActor))
                    {
                        // 내 손에 들려있던 상자 액터를 먼저 시원하게 파괴!
                        AttachedActor->Destroy();
                    }
                }

                this->Destroy();
            }
        },
        InDelay,
        false
    );

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
        FString::Printf(TEXT("[자동 소멸] [%s] %.1f초 뒤 자동 소멸 타이머 예약 완료!"), *GetName(), InDelay));
}

// Called to bind functionality to input
void ACustomerAI::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

}