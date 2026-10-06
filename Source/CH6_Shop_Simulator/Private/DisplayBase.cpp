// Fill out your copyright notice in the Description page of Project Settings.


#include "DisplayBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "CustomerAI.h" // 손님 호출을 위한 필수 인클루드
#include "EngineUtils.h"

// Sets default values
ADisplayBase::ADisplayBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// ★ 빈 루트를 먼저 만들어서 진짜 루트로 지정. DisplayMesh는 이 밑에
	// 자식으로 붙여서, 블루프린트에서 자유롭게 위치를 옮길 수 있게 함.
	DisplayRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DisplayRoot"));
	RootComponent = DisplayRoot;

	// ★ 지역 변수 대신 멤버로 만들어서, 배치 모드에서 재질을 바꿔치기할 수 있게 함
	DisplayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayMesh"));
	DisplayMesh->SetupAttachment(RootComponent);
	DisplayMesh->SetMobility(EComponentMobility::Movable); // 고스트 상태에서 위치가 계속 바뀌어야 하니 필수

	// 상품 하나만 올릴 진열 슬롯 위치
	DisplaySlot = CreateDefaultSubobject<USceneComponent>(TEXT("DisplaySlot"));
	DisplaySlot->SetupAttachment(RootComponent);

	// 진열대 중심 기준 위쪽으로 살짝 띄운 위치 (에디터에서 실제 조정)
	DisplaySlot->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	// ★ 손님 대기 위치. UBoxComponent라서 뷰포트에서 박스 크기를 눈으로 보면서
	// 조정 가능함 (사람 한 명 서있을 정도로). 콜리전은 꺼둠 - 실제 물리 충돌
	// 목적이 아니라, "이 자리에 뭔가 있는지" 검사할 때 크기 정보로만 씀.
	CustomerWaitSlot = CreateDefaultSubobject<UBoxComponent>(TEXT("CustomerWaitSlot"));
	CustomerWaitSlot->SetupAttachment(RootComponent);
	CustomerWaitSlot->SetBoxExtent(FVector(40.f, 40.f, 90.f));
	CustomerWaitSlot->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 진열대 중심(0,0,0) 기준 정면으로 70cm 튀어나온 바닥 위치로 설정 (필요시 조정 가능)
	CustomerWaitSlot->SetRelativeLocation(FVector(70.f, 0.f, 0.f));

	// ★ CustomerWaitSlot이랑 같은 위치에, 더 넓게 잡은 여유 공간 박스.
	// 벽 구석처럼 너무 좁은 곳에 손님이 끼지 않도록 미리 걸러내는 용도.
	ClearanceZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ClearanceZone"));
	ClearanceZone->SetupAttachment(RootComponent);
	ClearanceZone->SetBoxExtent(FVector(100.f, 100.f, 90.f)); // CustomerWaitSlot보다 넉넉하게
	ClearanceZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClearanceZone->SetRelativeLocation(FVector(70.f, 0.f, 0.f));

	// 상태 초기화: 처음엔 아무 상품도 안 올라와있는 상태
	OccupiedProduct = nullptr;
}

// Called when the game starts or when spawned
void ADisplayBase::BeginPlay()
{
	Super::BeginPlay();
}

void ADisplayBase::NotifyCustomersProductAvailable()
{
	// 지금 이 진열대에 상품이 정상적으로 입고되어 있는 자리라면 방송 시작!
	if (!IsSlotEmpty())
	{
		// 월드에 있는 모든 손님 AI들에게 이 사실을 전파합니다.
		for (TActorIterator<ACustomerAI> It(GetWorld()); It; ++It)
		{
			ACustomerAI* WaitingCustomer = *It;

			// ★ 아직 가게 앞을 지나가는 중이고, 목표 진열대가 없는 손님만 대상.
			// (이미 다른 진열대로 향하거나 계산 중인 손님은 건드리지 않음)
			if (WaitingCustomer && WaitingCustomer->TargetDisplay == nullptr
				&& WaitingCustomer->CurrentState == ECustomerState::PassingBy)
			{
				WaitingCustomer->TargetDisplay = this;
				WaitingCustomer->CurrentState = ECustomerState::WalkingToShelf;

				if (CustomerWaitSlot)
				{
					WaitingCustomer->MoveToLocation(CustomerWaitSlot->GetComponentLocation());
				}

				break; // 한 번에 여러 명이 몰려오지 않도록 안전장치로 1명만 우선 매칭 후 종료
			}
		}
	}
}

void ADisplayBase::EnterGhostMode()
{
	bIsGhost = true;

	// ★ 물리로 들고 다니는 대신, 예전 방식대로: 콜리전을 끄고 반투명 재질로만
	// 바꿔서 커서(카메라 시선)를 따라다니는 고스트로 만듦. 물리 시뮬레이션은
	// 아예 안 건드림 (계속 꺼진 상태 유지).
	DisplayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 원래 재질을 기억해뒀다가 나중에 복원
	OriginalMaterials.Empty();
	const int32 NumMaterials = DisplayMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		OriginalMaterials.Add(DisplayMesh->GetMaterial(i));
	}

	SetGhostValid(true); // 기본적으로 "배치 가능" 색(초록)으로 시작
}

void ADisplayBase::SetGhostValid(bool bIsValid)
{
	if (!bIsGhost)
	{
		return;
	}

	UMaterialInterface* MaterialToApply = bIsValid ? GhostMaterial : InvalidGhostMaterial;
	if (!MaterialToApply)
	{
		return;
	}

	const int32 NumMaterials = DisplayMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		DisplayMesh->SetMaterial(i, MaterialToApply);
	}
}

void ADisplayBase::ConfirmPlacement()
{
	bIsGhost = false;

	// 원래 재질로 복원
	for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
	{
		DisplayMesh->SetMaterial(i, OriginalMaterials[i]);
	}

	// 이제 실제로 기능하는 진열대니까 콜리전도 정상적으로 켬
	DisplayMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}