// Fill out your copyright notice in the Description page of Project Settings.
//
// 스플라인 위의 "킬존 지점"들을 관리하는 월드 서브시스템.
// - ADefenseWall(바리케이드)이 BeginPlay에서 자기 위치를 등록함
// - AMonsterBase는 "다음 킬존까지 왔는지"만 확인하면 됨 (매 프레임 전체 스캔 X)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SplinePathManager.generated.h"

class USplineComponent;
class ADefenseWall;

// 스플라인 위 하나의 킬존 지점 정보
USTRUCT()
struct FKillZoneEntry
{
	GENERATED_BODY()

	// 이 킬존이 스플라인 위 몇 미터(유닛) 지점에 있는지
	UPROPERTY()
	float Distance = 0.f;

	// 이 킬존에 배치된 바리케이드 (없으면 nullptr - 아직 안 만들어진 킬존일 수도 있음)
	UPROPERTY()
	TWeakObjectPtr<ADefenseWall> Barricade;
};

UCLASS()
class CH6_SHOP_SIMULATOR_API USplinePathManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ADefenseWall::BeginPlay에서 자기 자신을 등록할 때 호출
	// 같은 스플라인에 여러 개를 등록해도, Distance 순으로 자동 정렬됨
	void RegisterKillZone(USplineComponent* Spline, float Distance, ADefenseWall* Barricade);

	// 몬스터가 사용: Index번째 킬존 정보를 가져옴. 없으면 false 반환 (= 더 이상 킬존 없음 = 코어까지 직행)
	bool GetKillZoneAtIndex(USplineComponent* Spline, int32 Index, float& OutDistance, ADefenseWall*& OutBarricade) const;

private:
	// 스플라인별로 킬존 목록을 따로 관리 (여러 몬스터 레인이 있을 수 있으므로)
	TMap<TObjectPtr<USplineComponent>, TArray<FKillZoneEntry>> KillZonesBySpline;
};