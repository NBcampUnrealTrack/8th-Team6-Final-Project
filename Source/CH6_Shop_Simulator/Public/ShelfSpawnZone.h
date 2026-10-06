// Fill out your copyright notice in the Description page of Project Settings.
//
// 진열대가 처음 스폰될 구역. BP 안에 고정된 상대 위치로 넣는 대신, 레벨에
// 직접 박스 형태로 배치해서 "가게 바로 앞" 같은 원하는 자리를 눈으로 보면서
// 잡을 수 있게 함. 박스 범위 안에서 랜덤한 지점을 하나 뽑아주는 기능도 있어서,
// 여러 개를 연속 구매해도 정확히 같은 자리에 겹쳐서 스폰되는 걸 방지함.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShelfSpawnZone.generated.h"

class UBoxComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API AShelfSpawnZone : public AActor
{
	GENERATED_BODY()

public:
	AShelfSpawnZone();

	// 레벨 뷰포트에서 이 박스의 크기/위치를 직접 조정해서 원하는 구역을 잡으면 됨
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> SpawnVolume;

	// 이 박스 범위 안에서 랜덤한 월드 좌표 하나를 반환
	UFUNCTION(BlueprintCallable, Category = "Shelf Spawn Zone")
	FVector GetRandomSpawnPoint() const;
};