// Fill out your copyright notice in the Description page of Project Settings.
//
// 캐릭터에 붙는 "배치 모드" 전담 컴포넌트. 진열대를 클릭하면(새로 산 것이든
// 이미 놓여있던 것이든 상관없이) 배치 모드로 진입해서, 캐릭터 앞 고정 거리에
// 조준된 위치를 따라다님 (바닥이면 초록, 아니면 빨강). 다시 클릭하면 그 자리에
// 확정, 우클릭/ESC면 클릭하기 직전 있던 자리로 그대로 되돌아감.
//
// 대상은 ADisplayBase (무기 진열대/방어구 진열대 등이 전부 이 클래스를 상속받음).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShopPlacementComponent.generated.h"

class ADisplayBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CH6_SHOP_SIMULATOR_API UShopPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShopPlacementComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 진열대(새로 산 것이든 이미 놓여있던 것이든)를 클릭했을 때 호출: 배치 모드 시작
	UFUNCTION(BlueprintCallable, Category = "Shop Placement")
	void BeginPlacementMode(ADisplayBase* Target);

	// 좌클릭에 연결. 지금 조준된 위치에 확정 배치.
	UFUNCTION(BlueprintCallable, Category = "Shop Placement")
	void ConfirmPlacement();

	// 우클릭에 연결. 90도씩 수동으로 회전시킴 (위치는 그대로, 방향만 바뀜)
	UFUNCTION(BlueprintCallable, Category = "Shop Placement")
	void RotateGhost90();

	// 우클릭/ESC에 연결. 클릭하기 직전 있던 자리로 그대로 되돌림.
	UFUNCTION(BlueprintCallable, Category = "Shop Placement")
	void CancelPlacement();

	bool IsPlacing() const { return CurrentGhost != nullptr; }

	// 바닥 판정용 레이캐스트 최대 거리
	UPROPERTY(EditAnywhere, Category = "Shop Placement")
	float TraceDistance = 400.f;

private:
	UPROPERTY()
	TObjectPtr<ADisplayBase> CurrentGhost;

	// 배치 모드 시작 직전의 위치/회전. 취소하면 여기로 되돌아감.
	FTransform OriginalTransform;

	// 지금 조준된 위치가 배치 가능한 상태인지 (빨간색이면 false) - 확정 시 이걸로 막음
	bool bCurrentPositionValid = false;

	// ★ 고스트의 회전값. 카메라 방향을 따라가지 않고, 배치 모드 시작 시점의
	// 회전을 그대로 유지하다가 우클릭(RotateGhost90)으로만 90도씩 바뀜.
	float CurrentYaw = 0.f;

	void UpdateGhostPosition();
};