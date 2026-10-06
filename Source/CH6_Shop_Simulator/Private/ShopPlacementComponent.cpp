// Fill out your copyright notice in the Description page of Project Settings.

#include "ShopPlacementComponent.h"
#include "DisplayBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "DrawDebugHelpers.h"

UShopPlacementComponent::UShopPlacementComponent()
{
	// 배치 모드가 아닐 땐 매 프레임 아무것도 할 필요 없어서, CurrentGhost가
	// 있을 때만 실제 작업을 하도록 TickComponent 안에서 조건 체크함.
	PrimaryComponentTick.bCanEverTick = true;
}

void UShopPlacementComponent::BeginPlacementMode(ADisplayBase* Target)
{
	if (!Target || CurrentGhost)
	{
		return;
	}

	CurrentGhost = Target;
	OriginalTransform = Target->GetActorTransform(); // 취소하면 여기로 되돌아감
	CurrentYaw = Target->GetActorRotation().Yaw; // ★ 지금 방향을 그대로 유지 (카메라 따라 안 돌게)

	CurrentGhost->EnterGhostMode();
	UpdateGhostPosition(); // 배치 모드 진입하자마자 바로 한 번 위치 맞춰줌
}

void UShopPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentGhost)
	{
		UpdateGhostPosition();
	}
}

void UShopPlacementComponent::UpdateGhostPosition()
{
	ACH6_Shop_SimulatorCharacter* Character = Cast<ACH6_Shop_SimulatorCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UCameraComponent* Camera = Character->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return;
	}

	// ★ 카메라가 보는 방향(피치 포함) 그대로 한 줄기 레이를 쏨. 시선을 위로
	// 올리면 위로, 아래로 내리면 아래로 자연스럽게 따라감. 천장/바닥처럼 단단한
	// 지형에 콜리전이 있으면 레이 자체가 거기서 멈추니까, 뚫고 들어가는 일이
	// 원천적으로 안 생김 (별도로 막는 코드가 필요 없음).
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + (Camera->GetForwardVector() * TraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CurrentGhost);
	QueryParams.AddIgnoredActor(Character);

	// ★ 오브젝트 타입 쿼리만 쓰면 지정한 타입(Floor)이 아닌 것들(벽, 천장 등)은
	// 그냥 무시하고 통과해버려서, 벽 너머의 바닥까지 뚫고 들어가는 문제가 있었음.
	// 그래서 순서를 바꿈: 먼저 일반 콜리전(Visibility)으로 "제일 먼저 부딪히는
	// 지점"을 찾고(벽/천장/가구 다 막힘), 그다음 그 부딪힌 대상이 정말 Floor
	// 타입인지 확인하는 2단계 방식으로.
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);

	bool bIsRealFloor = false;
	if (bHit && Hit.GetComponent())
	{
		bIsRealFloor = (Hit.GetComponent()->GetCollisionObjectType() == ECC_GameTraceChannel3);
	}

	// ★ 메시 크기는 겹침 검사용 박스 크기 계산에만 씀 (액터 자체의 배치 위치에는
	// 더 이상 관여 안 함 - 그건 순수하게 블루프린트에서 DisplayMesh의 Relative
	// Location으로 결정하게 함).
	FVector BoxExtent(50.f, 50.f, 50.f); // 메시 정보 없을 때 대비한 기본값
	if (CurrentGhost->DisplayMesh && CurrentGhost->DisplayMesh->GetStaticMesh())
	{
		BoxExtent = CurrentGhost->DisplayMesh->GetStaticMesh()->GetBounds().BoxExtent * CurrentGhost->GetActorScale3D();
	}

	// ★ DisplayRoot(액터의 진짜 위치)는 레이가 부딪힌 바닥 지점 그 자체로 둠.
	// "메시를 얼마나 띄울지"는 코드가 추측하지 않고, 블루프린트에서 DisplayMesh의
	// Relative Location으로 직접 결정하게 함 (메시 종류마다 원점 위치가 다를 수
	// 있어서, 코드가 일괄적으로 추측하는 것보다 이게 더 정확하고 명확함).
	const FVector CandidateLocation = bHit ? Hit.Location : End;

	// 겹침 검사용 박스는 여전히 "대략 메시 중앙" 위치를 기준으로 삼음 (바닥
	// 지점에서 메시 절반 높이만큼 위 - 순수하게 검사 목적으로만 쓰는 근사치)
	const FVector OverlapTestCenter = CandidateLocation + FVector(0.f, 0.f, BoxExtent.Z);

	// ★ 카메라 방향을 따라가지 않고, CurrentYaw(우클릭으로만 바뀌는 값)를 그대로 씀
	const FRotator CandidateRotation(0.f, CurrentYaw, 0.f);

	CurrentGhost->SetActorLocation(CandidateLocation);
	CurrentGhost->SetActorRotation(CandidateRotation);

	// ★ 콜리전은 고스트 상태에서 꺼져있으니, 대신 "지금 이 자리에 다른 물체가
	// 이미 있는지"를 겹침 검사로 직접 확인함. 겹치는 게 있으면 배치 불가(빨강).
	// ★ 검사용 박스를 실제 크기보다 살짝(약 5%) 줄여서 씀 - 바닥에 정확히 표면
	// 대 표면으로 딱 맞닿아 있으면, 부동소수점 오차 때문에 "살짝 겹친 것"으로
	// 오판하는 경우가 있어서(마우스를 미세하게 움직일 때마다 판정이 오락가락하는
	// 원인이었음), 여유를 살짝 줘서 이런 잘못된 판정을 방지함.
	const FVector OverlapTestExtent = BoxExtent * 0.95f;
	const bool bOverlapping = GetWorld()->OverlapAnyTestByChannel(
		OverlapTestCenter, CandidateRotation.Quaternion(), ECC_Visibility,
		FCollisionShape::MakeBox(OverlapTestExtent), QueryParams);

	// ★ 진열대와 손님 대기 위치(CustomerWaitSlot) 사이에 벽 같은 게 끼어있으면
	// 안 됨. 방향을 돌렸을 때 대기 위치가 벽 너머로 가버리면, 손님 AI가 그
	// 지점까지 못 찾아가서 영원히 멈춰있는 문제가 생기기 때문에 여기서 미리 막음.
	bool bWaitSlotBlocked = false;
	bool bWaitSlotOccupied = false;
	if (CurrentGhost->CustomerWaitSlot)
	{
		const FVector WaitSlotLocation = CurrentGhost->CustomerWaitSlot->GetComponentLocation();

		FHitResult WaitSlotHit;
		// ★ 시작/끝 지점을 서로를 향해 살짝(10유닛) 당겨서 검사. 정확히 표면에서
		// 표면까지 검사하면 진열대 자기 자신의 표면 경계에 스치듯 걸려서
		// 오탐이 생길 수 있어서, 약간의 여유를 줌.
		const FVector ToWaitSlot = (WaitSlotLocation - CandidateLocation).GetSafeNormal();
		const FVector TraceStartPoint = CandidateLocation + (ToWaitSlot * 10.f);
		const FVector TraceEndPoint = WaitSlotLocation - (ToWaitSlot * 10.f);
		bWaitSlotBlocked = GetWorld()->LineTraceSingleByChannel(
			WaitSlotHit, TraceStartPoint, TraceEndPoint, ECC_Visibility, QueryParams);

		// ★ 시야가 막혔는지랑 별개로, 그 대기 위치 자체가 다른 물체(다른 진열대의
		// 대기 위치, 벽 등)랑 겹쳐서 실제로 손님이 서있을 공간이 없는지도 확인.
		// 박스 크기는 코드에 박아넣지 않고, CustomerWaitSlot 자체의 크기를 그대로
		// 씀 - 진열대 종류마다 블루프린트 뷰포트에서 눈으로 보며 다르게 조정 가능.
		const FVector WaitSlotBoxExtent = CurrentGhost->CustomerWaitSlot->GetScaledBoxExtent();
		bWaitSlotOccupied = GetWorld()->OverlapAnyTestByChannel(
			WaitSlotLocation, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeBox(WaitSlotBoxExtent), QueryParams);

		// ★ 진짜 확실한 검사: 콜리전 박스는 "물리적으로 뭔가 있는지"만 보는 거라,
		// 네브메시 자체가 통로 폭 등의 이유로 막혀있는 경우까지는 못 잡음. 대신
		// 내비게이션 시스템한테 "이 지점이 실제로 네브메시 위에 있는지"를 직접
		// 물어봐서, 손님 AI가 진짜로 갈 수 있는 자리인지 확실하게 확인함.
		// ★ 대부분의 상점 경영 시뮬레이터가 쓰는 방식대로, "주변에 충분한 여유
		// 공간이 있는지"를 박스로 확인함 (벽 구석처럼 너무 좁은 곳에 끼는 걸
		// 막는 용도). 구(Sphere)로 하면 바닥까지 파고들어서 항상 걸려버리는
		// 문제가 있어서, CustomerWaitSlot과 똑같은 방식으로 박스를 씀 -
		// ClearanceZone 자체의 위치/크기를 그대로 사용 (뷰포트에서 조정 가능).
		if (CurrentGhost->ClearanceZone)
		{
			const FVector ClearanceLocation = CurrentGhost->ClearanceZone->GetComponentLocation();
			const FVector ClearanceExtent = CurrentGhost->ClearanceZone->GetScaledBoxExtent();

			const bool bHasClearance = !GetWorld()->OverlapAnyTestByChannel(
				ClearanceLocation, CandidateRotation.Quaternion(), ECC_Visibility,
				FCollisionShape::MakeBox(ClearanceExtent), QueryParams);

			if (!bHasClearance)
			{
				bWaitSlotOccupied = true;
			}

			// ★ ClearanceZone도 눈에 보이게 디버그 박스로 표시 (더 큰 바깥쪽 박스)
			const FColor ClearanceDebugColor = bHasClearance ? FColor::Green : FColor::Red;
			DrawDebugBox(GetWorld(), ClearanceLocation, ClearanceExtent, CandidateRotation.Quaternion(),
				ClearanceDebugColor, false, -1.f, 0, 1.f);
		}

		// ★ 디버그 시각화: 대기 위치를 네모난 박스로 표시해서 눈으로 확인 가능하게 함.
		// (배치 가능하면 초록, 막혔거나 겹치면 빨강 - 폴리싱 단계에서 실제
		// 바닥 데칼 등으로 교체하면 되고, 그전까진 이 표시로 감 잡으면 됨)
		const FColor DebugColor = (!bWaitSlotBlocked && !bWaitSlotOccupied) ? FColor::Green : FColor::Red;
		DrawDebugBox(GetWorld(), WaitSlotLocation, WaitSlotBoxExtent, DebugColor, false, -1.f, 0, 2.f);
	}

	bCurrentPositionValid = bIsRealFloor && !bOverlapping && !bWaitSlotBlocked && !bWaitSlotOccupied;
	CurrentGhost->SetGhostValid(bCurrentPositionValid);
}

void UShopPlacementComponent::RotateGhost90()
{
	if (!CurrentGhost)
	{
		return;
	}

	CurrentYaw += 90.f;
	if (CurrentYaw >= 360.f)
	{
		CurrentYaw -= 360.f;
	}

	// 다음 Tick까지 안 기다리고 바로 반영 (즉각적인 반응감을 위해)
	CurrentGhost->SetActorRotation(FRotator(0.f, CurrentYaw, 0.f));
}

void UShopPlacementComponent::ConfirmPlacement()
{
	if (!CurrentGhost)
	{
		return;
	}

	// ★ 빨간색(배치 불가) 상태면 확정 자체를 막음. 계속 고스트 모드 유지되니까
	// 플레이어는 다른 유효한 자리를 찾아서 다시 클릭하면 됨.
	if (!bCurrentPositionValid)
	{
		return;
	}

	CurrentGhost->ConfirmPlacement(); // 콜리전 켜고 재질 복원
	CurrentGhost = nullptr;
}

void UShopPlacementComponent::CancelPlacement()
{
	if (!CurrentGhost)
	{
		return;
	}

	// ★ 새로 산 것이든 원래 있던 것이든 상관없이, 배치 모드 시작 직전의 자리로
	// 그대로 되돌림. 돈은 이미 구매 시점에 확정된 것이라 여기서 환불하지 않음.
	CurrentGhost->SetActorTransform(OriginalTransform);
	CurrentGhost->ConfirmPlacement();

	CurrentGhost = nullptr;
}