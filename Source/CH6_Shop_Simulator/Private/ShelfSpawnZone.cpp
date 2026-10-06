// Fill out your copyright notice in the Description page of Project Settings.

#include "ShelfSpawnZone.h"
#include "Components/BoxComponent.h"

AShelfSpawnZone::AShelfSpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnVolume"));
	RootComponent = SpawnVolume;
	SpawnVolume->SetBoxExtent(FVector(100.f, 100.f, 20.f)); // 기본 크기, 레벨에서 자유롭게 조정
	SpawnVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 순수 마커용, 실제 충돌은 필요 없음
}

FVector AShelfSpawnZone::GetRandomSpawnPoint() const
{
	if (!SpawnVolume)
	{
		return GetActorLocation();
	}

	// ★ 가로/세로(X, Y)만 랜덤하게 뽑고, 높이(Z)는 박스 위치에 고정함.
	// 셋 다 랜덤하게 뽑으면, 박스 중심을 바닥 높이에 맞춰놔도 절반은 바닥보다
	// 위(공중에 뜸), 절반은 바닥보다 아래(파묻힘)로 나오는 문제가 있었음.
	const FVector Extent = SpawnVolume->GetScaledBoxExtent();
	const FVector RandomLocalOffset(
		FMath::RandRange(-Extent.X, Extent.X),
		FMath::RandRange(-Extent.Y, Extent.Y),
		0.f);

	return SpawnVolume->GetComponentTransform().TransformPosition(RandomLocalOffset);
}