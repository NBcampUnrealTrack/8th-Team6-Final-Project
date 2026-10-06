// Fill out your copyright notice in the Description page of Project Settings.


#include "CounterBase.h"
#include "CustomerAI.h"
#include "Components/StaticMeshComponent.h"
// Sets default values
ACounterBase::ACounterBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// 가구 메쉬 루트 설정
	CounterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CounterMesh"));
	RootComponent = CounterMesh;

	// 손님 대기선 컴포넌트 생성 (계산대 앞 70cm 부근)
	CustomerWaitSlot = CreateDefaultSubobject<USceneComponent>(TEXT("CustomerWaitSlot"));
	CustomerWaitSlot->SetupAttachment(RootComponent);
	CustomerWaitSlot->SetRelativeLocation(FVector(70.f, 0.f, 0.f));

	// 돈 올려놓는 위치 컴포넌트 생성 (계산대 테이블 위 상단)
	MoneySpawnSlot = CreateDefaultSubobject<USceneComponent>(TEXT("MoneySpawnSlot"));
	MoneySpawnSlot->SetupAttachment(RootComponent);
	MoneySpawnSlot->SetRelativeLocation(FVector(0.f, 0.f, 80.f)); // 테이블 높이만큼 위로
}

// Called when the game starts or when spawned
void ACounterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACounterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// CounterBase.cpp 내부

int32 ACounterBase::JoinQueue(ACustomerAI* NewCustomer)
{
	if (!NewCustomer) return -1;

	// 1. 대기열 배열 맨 뒤에 이 손님을 추가합니다.
	CustomerQueue.Add(NewCustomer);

	// 2. 이 손님이 몇 번째 자리에 서게 되었는지 번호(인덱스)를 반환합니다.
	// (예: 첫 번째 손님이면 0번, 두 번째 손님이면 1번...)
	return CustomerQueue.Num() - 1;
}

void ACounterBase::PopQueue()
{
	if (CustomerQueue.Num() == 0) return;

	// 1. 맨 앞(0번) 손님은 계산이 끝났으므로 줄에서 제거합니다.
	CustomerQueue.RemoveAt(0);

	// 2. ★ [도미노 효과]: 뒤에 서 있던 손님들에게 "너네 이제 한 칸씩 앞으로 당겨서 걸어가!"라고 지시합니다.
	for (int32 i = 0; i < CustomerQueue.Num(); i++)
	{
		if (CustomerQueue[i])
		{
			// 새로운 줄 번호(i)에 맞춰서 이동할 목표 좌표를 다시 계산합니다.
			// 가구 정면 기준 (70cm + 줄번호 * 70cm) 만큼 뒤로 밀린 좌표를 걷게 만듭니다.
			FVector NewWaitLocation = CustomerWaitSlot->GetComponentLocation() + (GetActorForwardVector() * (i * 70.f));

			// 손님아, 한 칸 앞으로 저벅저벅 이동해라!
			CustomerQueue[i]->MoveToLocation(NewWaitLocation);
		}
	}
}