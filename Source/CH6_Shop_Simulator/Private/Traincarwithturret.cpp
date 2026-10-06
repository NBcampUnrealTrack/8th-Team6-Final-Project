// Fill out your copyright notice in the Description page of Project Settings.

#include "TrainCarWithTurret.h"
#include "TowerBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

ATrainCarWithTurret::ATrainCarWithTurret()
{
	PrimaryActorTick.bCanEverTick = false;

	CarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarMesh"));
	RootComponent = CarMesh;

	FrontCouplingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FrontCouplingPoint"));
	FrontCouplingPoint->SetupAttachment(RootComponent);
	FrontCouplingPoint->SetRelativeLocation(FVector(100.f, 0.f, 0.f)); // 화물칸 크기에 맞게 조정

	RearCouplingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RearCouplingPoint"));
	RearCouplingPoint->SetupAttachment(RootComponent);
	RearCouplingPoint->SetRelativeLocation(FVector(-100.f, 0.f, 0.f));

	TurretMount = CreateDefaultSubobject<USceneComponent>(TEXT("TurretMount"));
	TurretMount->SetupAttachment(RootComponent);
	TurretMount->SetRelativeLocation(FVector(0.f, 0.f, 50.f)); // 화물칸 위쪽
}

void ATrainCarWithTurret::BeginPlay()
{
	Super::BeginPlay();

	if (!TurretClass || !TurretMount)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnedTurret = GetWorld()->SpawnActor<ATowerBase>(
		TurretClass, TurretMount->GetComponentLocation(), TurretMount->GetComponentRotation(), SpawnParams);

	if (SpawnedTurret)
	{
		// ★ 이 Attach 한 줄이 핵심. 이후로는 화물칸이 어디로 움직이든
		// 터렛이 자동으로 따라가고, ATowerBase의 감지/조준/발사 코드는
		// 전혀 손댈 필요가 없음 (전부 "그 순간의 월드 좌표" 기준으로 계산되므로).
		SpawnedTurret->AttachToComponent(TurretMount, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
}