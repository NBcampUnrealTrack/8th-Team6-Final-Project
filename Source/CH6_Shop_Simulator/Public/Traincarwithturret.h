// Fill out your copyright notice in the Description page of Project Settings.
//
// 기차 뒤에 연결하는 화물칸. 위에 ATowerBase(또는 그 자식 클래스)를 스폰해서 부착함.
// ATowerBase의 감지/회전/발사 로직은 전부 "지금 이 순간의 월드 좌표" 기준으로 계산되므로,
// 이 화물칸이 기차에 붙어서 움직여도 터렛 코드를 전혀 안 건드려도 자동으로 잘 작동함
// (언리얼이 부착된 자식의 월드 좌표를 매 프레임 자동으로 갱신해주기 때문).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainCarWithTurret.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ATowerBase;

UCLASS()
class CH6_SHOP_SIMULATOR_API ATrainCarWithTurret : public AActor
{
	GENERATED_BODY()

public:
	ATrainCarWithTurret();

protected:
	virtual void BeginPlay() override;

public:
	// 화물칸 몸체 (지금은 큐브로 대충, 나중에 메시 교체)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CarMesh;

	// 이 화물칸 앞쪽에 다음 화물칸/기차를 연결할 지점 (체인 연결용)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> FrontCouplingPoint;

	// 이 화물칸 뒤쪽에 또 다른 화물칸을 연결할 지점 (여러 칸 연결 확장 대비)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> RearCouplingPoint;

	// 터렛이 스폰되어 부착될 위치
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> TurretMount;

	// 스폰할 터렛 클래스 (ATowerBase 또는 그 자식 블루프린트 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Train Car")
	TSubclassOf<ATowerBase> TurretClass;

private:
	UPROPERTY()
	TObjectPtr<ATowerBase> SpawnedTurret;
};