// Fill out your copyright notice in the Description page of Project Settings.

#include "FogBarrier.h"
#include "Components/BoxComponent.h"
#include "TrainPawn.h"
#include "SplineFollowComponent.h"
#include "EngineUtils.h"

AFogBarrier::AFogBarrier()
{
	// 페이드가 진행되는 동안에만 필요하므로, 평소엔 Tick 안에서 바로 리턴함
	PrimaryActorTick.bCanEverTick = true;

	BlockVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockVolume"));
	RootComponent = BlockVolume;
	BlockVolume->SetBoxExtent(FVector(50.f, 300.f, 200.f)); // 길 폭에 맞게 조정
	BlockVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// ★ 콜리전 프리셋은 블루프린트에서 지정할 것.
	// 플레이어만 막고 몬스터는 통과해야 하므로, 커스텀 프리셋으로
	// 플레이어 채널만 Block, 몬스터 채널은 Ignore로 설정하면 됨.

	FogRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FogRoot"));
	FogRoot->SetupAttachment(RootComponent);
}

void AFogBarrier::BeginPlay()
{
	Super::BeginPlay();

	// ★ 시작할 때 기차의 이동 가능 거리를 이 장벽 앞까지로 제한.
	// (기차는 콜리전으로 안 막히므로 거리로 제어해야 함)
	if (TrainMaxDistanceWhileClosed >= 0.f)
	{
		for (TActorIterator<ATrainPawn> It(GetWorld()); It; ++It)
		{
			if (ATrainPawn* Train = *It)
			{
				if (Train->SplineFollowComp)
				{
					Train->SplineFollowComp->MaxAllowedDistance = TrainMaxDistanceWhileClosed;
				}
			}
		}
	}
}

void AFogBarrier::UpdateForDay(int32 CurrentDay)
{
	// 이미 열렸거나 아직 날짜가 안 됐으면 아무것도 안 함
	if (bIsOpen || bIsFading || CurrentDay < OpenOnDay)
	{
		return;
	}

	// ★ 길막이는 즉시 해제하고, 안개는 서서히 걷히게 함.
	// (막힌 게 먼저 풀려야 그날 밤 몬스터가 바로 지나올 수 있음)
	bIsFading = true;
	FadeElapsed = 0.f;

	if (BlockVolume)
	{
		BlockVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 기차 이동 제한도 해제
	if (TrainMaxDistanceWhileClosed >= 0.f)
	{
		for (TActorIterator<ATrainPawn> It(GetWorld()); It; ++It)
		{
			if (ATrainPawn* Train = *It)
			{
				if (Train->SplineFollowComp)
				{
					Train->SplineFollowComp->MaxAllowedDistance = -1.f; // 제한 없음
				}
			}
		}
	}

	OnBarrierOpened();
}

void AFogBarrier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 페이드 중이 아니면 할 일 없음
	if (!bIsFading)
	{
		return;
	}

	FadeElapsed += DeltaTime;

	const float SafeDuration = FMath::Max(0.1f, FadeOutDuration);
	const float Density = FMath::Clamp(1.f - (FadeElapsed / SafeDuration), 0.f, 1.f);

	// 실제 안개 표현은 블루프린트에서 이 값을 받아 처리
	OnFogDensityChanged(Density);

	if (Density <= 0.f)
	{
		bIsFading = false;
		bIsOpen = true;

		// 완전히 걷혔으면 보이는 것도 전부 숨김
		SetActorHiddenInGame(true);
	}
}