// Fill out your copyright notice in the Description page of Project Settings.
//
// 기차 레일의 분기 지점. 두 스플라인(예: 바깥 순환 루트 <-> 안쪽 지름길)이
// 만나는 자리에 배치하고, 각각 어느 스플라인의 몇 번째 지점끼리 연결되는지
// 지정해두면 됨. 기차가 이 범위 안에 들어오면 Q키로 갈아탈 수 있게 됨.
//
// 배치 방법:
// 1. 레벨에서 두 레일 스플라인이 서로 만나는(가까워지는) 지점에 이 액터를 놓음
// 2. RailA/RailB에 각각의 스플라인을 가진 액터를 지정
// 3. DistanceOnRailA/DistanceOnRailB에 "그 스플라인에서 이 분기점이 몇 cm 지점인지" 입력
//    (스플라인 컴포넌트를 선택하면 각 포인트의 거리를 확인할 수 있음)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RailSwitchPoint.generated.h"

class USphereComponent;
class USplineComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API ARailSwitchPoint : public AActor
{
	GENERATED_BODY()

public:
	ARailSwitchPoint();

	virtual void Tick(float DeltaTime) override;

	// 범위를 게임 중에 노란 구체로 표시 (배치/테스트용, 완성되면 꺼도 됨)
	UPROPERTY(EditAnywhere, Category = "Rail Switch")
	bool bShowRangeDebug = true;

	// 기차가 이 범위 안에 있을 때만 분기 가능. 뷰포트에서 크기 조정 가능.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> SwitchZone;

	// 연결되는 두 레일 (스플라인 컴포넌트를 가진 액터를 지정)
	UPROPERTY(EditInstanceOnly, Category = "Rail Switch")
	TObjectPtr<AActor> RailA;

	UPROPERTY(EditInstanceOnly, Category = "Rail Switch")
	TObjectPtr<AActor> RailB;

	// ★ 거리는 직접 입력할 필요 없음! 이 액터의 위치에서 각 스플라인의 가장
	// 가까운 지점을 코드가 알아서 찾아냄. 그러니 레벨에서 두 레일이 만나는
	// 자리에 이 액터를 놓기만 하면 됨.

	// 지금 기차가 타고 있는 스플라인을 넣으면, 갈아탈 반대편 스플라인과 그 거리를 알려줌.
	// 어느 쪽에도 해당 안 되면 false 반환 (이 분기점과 무관한 레일을 타고 있는 경우).
	bool GetOppositeRail(USplineComponent* CurrentSpline, USplineComponent*& OutTargetSpline, float& OutTargetDistance) const;

	// 이 분기점이 커버하는 범위 (기차가 이 안에 있어야 분기 가능)
	float GetSwitchRadius() const;
};