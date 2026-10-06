// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"
#include "Components/MeshComponent.h"

UInteractableComponent::UInteractableComponent()
{
	// 이 컴포넌트 자체는 매 프레임 할 일이 없음. 플레이어 쪽에서 주기적으로
	// 라인트레이스를 쏴서 이 컴포넌트를 찾아내고 SetHighlighted를 불러줌.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UInteractableComponent::IsValidHitComponent(const UPrimitiveComponent* HitComponent) const
{
	// 정확한 컴포넌트를 요구하지 않으면 아무 데나 맞아도 통과
	if (!bRequireExactMeshHit || HighlightMeshNames.Num() == 0)
	{
		return true;
	}

	// 지정된 컴포넌트를 직접 맞았을 때만 인정
	// (예: 기차 바닥이 아니라 터렛을 정확히 봐야만 탑승 UI가 뜨게)
	return HitComponent && HighlightMeshNames.Contains(HitComponent->GetFName());
}

void UInteractableComponent::SetHighlighted(bool bHighlighted)
{
	// ★ 예전엔 "이미 같은 상태면 그냥 리턴"하는 방어 코드가 있었는데, 내부
	// 플래그와 실제 머티리얼 상태가 어긋나면(예: 탑승/하차로 대상이 강제로
	// 바뀌는 경우) 다시 켜라고 해도 무시되어버리는 문제가 있었음.
	// 대상이 바뀔 때만 호출되는 함수라 매번 적용해도 부담이 없으므로 그냥 항상 적용.
	bIsHighlighted = bHighlighted;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// ★ 액터에 달린 모든 메시 컴포넌트에 적용. 액터 하나에 메시가 여러 개인
	// 경우(빌드 타일의 타일 + 간판 등)에도 전부 같이 하이라이트되게 하기 위함.
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh)
		{
			continue;
		}

		// ★ HighlightMeshNames가 비어있으면 전부 적용, 이름이 지정되어 있으면
		// 그 목록에 있는 컴포넌트만 적용. (기차처럼 액터는 크지만 실제로
		// 상호작용하는 부분은 일부분뿐인 경우를 위함)
		if (HighlightMeshNames.Num() > 0 && !HighlightMeshNames.Contains(Mesh->GetFName()))
		{
			continue;
		}

		// 켤 때는 지정된 머티리얼을, 끌 때는 nullptr을 넣어서 원래대로 되돌림
		Mesh->SetOverlayMaterial(bHighlighted ? HighlightMaterial : nullptr);
	}

	// 외곽선 외에 추가 연출이 필요한 쪽(예: 타워 사거리 표시)에 알림
	OnHighlightChanged.Broadcast(bHighlighted);
}