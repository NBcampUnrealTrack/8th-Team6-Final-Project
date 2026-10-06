// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomerZone.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"

ACustomerZone::ACustomerZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneVolume"));
	RootComponent = ZoneVolume;
	ZoneVolume->SetBoxExtent(FVector(150.f, 150.f, 100.f)); // 기본 크기, 레벨에서 자유롭게 조정
	ZoneVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 순수 마커용
}

FVector ACustomerZone::GetRandomPoint() const
{
	if (!ZoneVolume)
	{
		return GetActorLocation();
	}

	// 가로/세로만 랜덤하게 뽑고 높이는 마커 위치에 고정.
	// (Z까지 랜덤하면 손님이 공중에 뜨거나 바닥에 박혀서 스폰됨)
	const FVector Extent = ZoneVolume->GetScaledBoxExtent();
	const FVector RandomLocalOffset(
		FMath::RandRange(-Extent.X, Extent.X),
		FMath::RandRange(-Extent.Y, Extent.Y),
		0.f);

	return ZoneVolume->GetComponentTransform().TransformPosition(RandomLocalOffset);
}

bool ACustomerZone::IsLocationInside(const FVector& WorldLocation) const
{
	if (!ZoneVolume)
	{
		return false;
	}

	// 월드 좌표를 이 박스의 로컬 좌표로 바꾼 다음, 각 축이 범위 안에 있는지 확인.
	// (박스가 회전되어 있어도 정확하게 판정됨)
	const FVector LocalPoint = ZoneVolume->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = ZoneVolume->GetScaledBoxExtent();

	return FMath::Abs(LocalPoint.X) <= Extent.X
		&& FMath::Abs(LocalPoint.Y) <= Extent.Y
		&& FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

ACustomerZone* ACustomerZone::FindZone(const UObject* WorldContext, ECustomerZoneType InType)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACustomerZone> It(World); It; ++It)
	{
		if (It->ZoneType == InType)
		{
			return *It;
		}
	}
	return nullptr;
}