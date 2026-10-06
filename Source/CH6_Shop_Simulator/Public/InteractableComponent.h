// Fill out your copyright notice in the Description page of Project Settings.
//
// 상호작용 가능한 액터에 붙이는 컴포넌트. 이 컴포넌트가 붙어있는 액터를 플레이어가
// 바라보면 자동으로 하이라이트가 켜지고, 화면에 안내 문구가 뜸.
//
// 사용법:
// 1. 상호작용 가능한 액터의 블루프린트에서 이 컴포넌트를 추가
// 2. HighlightMaterial에 하이라이트용 오버레이 머티리얼 지정
// 3. InteractionText에 안내 문구 입력 (예: "좌클릭으로 건설")
// 4. 상황에 따라 문구/가능 여부를 바꾸고 싶으면 블루프린트에서
//    GetInteractionText / CanInteract를 오버라이드

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

class UMaterialInterface;

// 하이라이트 상태가 바뀔 때 알려주는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHighlightChanged, bool, bHighlighted);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CH6_SHOP_SIMULATOR_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	// 플레이어가 이 액터를 바라볼 때 화면에 뜰 안내 문구
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	FText InteractionText = FText::FromString(TEXT("좌클릭으로 상호작용"));

	// 바라볼 때 메시에 덧씌울 하이라이트 머티리얼 (오버레이 방식)
	UPROPERTY(EditAnywhere, Category = "Interactable")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	// ★ 하이라이트할 메시 컴포넌트 이름 목록. 비워두면 액터의 모든 메시가
	// 빛나고, 이름을 지정하면 그 컴포넌트들만 빛남.
	// 예: 기차의 경우 몸통 전체가 아니라 터렛 부분만 빛나게 하고 싶을 때
	// ["TurretHeadComp", "TurretVisualMesh"]처럼 입력.
	UPROPERTY(EditAnywhere, Category = "Interactable")
	TArray<FName> HighlightMeshNames;

	// ★ true면, 위 HighlightMeshNames에 있는 컴포넌트를 직접 바라봤을 때만
	// 상호작용 대상으로 인식함. false(기본)면 액터의 아무 부분이나 바라봐도 인식.
	// 예: 기차는 터렛을 정확히 봐야만 탑승 UI가 뜨게 하고 싶을 때 true로.
	UPROPERTY(EditAnywhere, Category = "Interactable")
	bool bRequireExactMeshHit = false;

	// 라인트레이스로 맞은 컴포넌트가 상호작용 대상으로 인정되는지 확인
	bool IsValidHitComponent(const class UPrimitiveComponent* HitComponent) const;

	// ★ 지금 상호작용이 가능한 상태인지. 기본은 항상 true지만, 블루프린트에서
	// 오버라이드해서 상황별로 다르게 만들 수 있음 (예: 상품이 올라간 진열대는
	// 재배치 불가이므로 false를 반환하게).
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
	bool CanInteract() const;
	virtual bool CanInteract_Implementation() const { return true; }

	// ★ 화면에 표시할 문구. 기본은 InteractionText를 그대로 반환하지만,
	// 블루프린트에서 오버라이드해서 상황별로 다른 문구를 낼 수 있음.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactable")
	FText GetInteractionText() const;
	virtual FText GetInteractionText_Implementation() const { return InteractionText; }

	// 외곽선 켜기/끄기. 이 액터에 달린 모든 메시 컴포넌트에 한꺼번에 적용됨
	// (예: 빌드 타일의 타일 메시 + 그 위 간판 메시까지 전부).
	UFUNCTION(BlueprintCallable, Category = "Interactable")
	void SetHighlighted(bool bHighlighted);

	// ★ 하이라이트 상태가 바뀔 때마다 발생. 외곽선 말고 추가 연출이 필요할 때
	// 이 이벤트를 받아서 처리하면 됨 (예: 타워를 바라보면 사거리 원도 같이 표시).
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FOnHighlightChanged OnHighlightChanged;

	bool IsHighlighted() const { return bIsHighlighted; }

private:
	bool bIsHighlighted = false;
};