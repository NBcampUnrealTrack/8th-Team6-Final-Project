// Fill out your copyright notice in the Description page of Project Settings.
//
// 상점 오픈 간판. 플레이어가 상호작용하면 GameTimeManager에게 "오픈"을 알려서
// 낮 시간이 흐르기 시작하고 손님 스포너가 켜짐.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenSignActor.generated.h"

class UStaticMeshComponent;
class AGameTimeManager;

UCLASS()
class CH6_SHOP_SIMULATOR_API AOpenSignActor : public AActor
{
	GENERATED_BODY()

public:
	AOpenSignActor();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SignMesh;

	// 레벨에 배치된 GameTimeManager 지정
	UPROPERTY(EditAnywhere, Category = "Open Sign")
	TObjectPtr<AGameTimeManager> GameTimeManager;

	// 캐릭터의 상호작용 함수에서 이 함수를 호출하면 됨
	UFUNCTION(BlueprintCallable, Category = "Open Sign")
	void TryOpen();
};