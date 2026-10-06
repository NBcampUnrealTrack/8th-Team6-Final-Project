// Fill out your copyright notice in the Description page of Project Settings.

#include "RailSwitchPoint.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"

ARailSwitchPoint::ARailSwitchPoint()
{
	PrimaryActorTick.bCanEverTick = true; // 범위 시각화를 위해 켬

	SwitchZone = CreateDefaultSubobject<USphereComponent>(TEXT("SwitchZone"));
	RootComponent = SwitchZone;
	SwitchZone->SetSphereRadius(300.f); // 기차가 이 반경 안에 들어오면 분기 가능
	SwitchZone->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 순수 범위 표시용
}

void ARailSwitchPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 배치/테스트할 때 범위를 눈으로 확인하기 위한 표시. 완성되면 꺼도 됨.
	if (bShowRangeDebug)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GetSwitchRadius(), 16, FColor::Yellow, false, -1.f, 0, 2.f);
	}
}

float ARailSwitchPoint::GetSwitchRadius() const
{
	return SwitchZone ? SwitchZone->GetScaledSphereRadius() : 300.f;
}

bool ARailSwitchPoint::GetOppositeRail(USplineComponent* CurrentSpline, USplineComponent*& OutTargetSpline, float& OutTargetDistance) const
{
	USplineComponent* SplineA = RailA ? RailA->FindComponentByClass<USplineComponent>() : nullptr;
	USplineComponent* SplineB = RailB ? RailB->FindComponentByClass<USplineComponent>() : nullptr;

	if (!SplineA || !SplineB || !CurrentSpline)
	{
		return false;
	}

	// 지금 A를 타고 있으면 B로, B를 타고 있으면 A로 보내줌
	USplineComponent* TargetSpline = nullptr;
	if (CurrentSpline == SplineA)
	{
		TargetSpline = SplineB;
	}
	else if (CurrentSpline == SplineB)
	{
		TargetSpline = SplineA;
	}
	else
	{
		return false; // 이 분기점과 무관한 레일을 타고 있음
	}

	// ★ 거리를 수동으로 입력받지 않고, 이 분기점 액터의 위치에서 목표 스플라인의
	// "가장 가까운 지점"을 자동으로 찾아냄. 레벨에서 분기점을 두 레일이 만나는
	// 자리에 놓기만 하면 알아서 맞는 값이 나옴.
	const FVector MyLocation = GetActorLocation();
	const float InputKey = TargetSpline->FindInputKeyClosestToWorldLocation(MyLocation);

	OutTargetSpline = TargetSpline;
	OutTargetDistance = TargetSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	return true;
}