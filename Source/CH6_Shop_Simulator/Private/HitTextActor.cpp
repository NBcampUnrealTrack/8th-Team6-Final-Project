// Fill out your copyright notice in the Description page of Project Settings.

#include "HitTextActor.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

AHitTextActor::AHitTextActor()
{
	PrimaryActorTick.bCanEverTick = true;

	TextRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TextRoot"));
	RootComponent = TextRoot;
}

void AHitTextActor::InitHitText()
{
	// ★ 같은 자리에서 여러 개가 떠도 겹쳐 보이지 않도록 위치를 조금씩 흩어뜨림
	const FVector RandomOffset(
		FMath::RandRange(-RandomOffsetRange.X, RandomOffsetRange.X),
		FMath::RandRange(-RandomOffsetRange.Y, RandomOffsetRange.Y),
		FMath::RandRange(-RandomOffsetRange.Z, RandomOffsetRange.Z));

	SetActorLocation(GetActorLocation() + RandomOffset);

	BaseScale = GetActorScale3D();
	ElapsedTime = 0.f;

	// 수명이 다하면 스스로 사라짐
	SetLifeSpan(LifeTime);
}

void AHitTextActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	// 위로 천천히 떠오름
	AddActorWorldOffset(FVector(0.f, 0.f, FloatSpeed * DeltaTime));

	// ★ 항상 카메라를 정면으로 바라보게 함 (빌보드).
	// 어느 방향에서 봐도 글자가 제대로 읽히도록.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
			FRotator LookRotation = (CameraLocation - GetActorLocation()).Rotation();

			// 스폰 시 정해둔 기울기를 유지해서 만화적인 느낌을 살림
			LookRotation.Roll = GetActorRotation().Roll;
			SetActorRotation(LookRotation);
		}
	}

	// ★ 등장 순간 살짝 커졌다가 원래 크기로 돌아오는 "팝" 연출
	if (ElapsedTime < PopDuration && PopDuration > 0.f)
	{
		const float PopAlpha = ElapsedTime / PopDuration;
		const float CurrentScale = FMath::Lerp(PopScale, 1.f, PopAlpha);
		SetActorScale3D(BaseScale * CurrentScale);
	}

	// 수명의 마지막 절반 동안 서서히 투명해짐 (실제 적용은 블루프린트에서)
	const float FadeStart = LifeTime * 0.5f;
	const float Alpha = (ElapsedTime < FadeStart)
		? 1.f
		: FMath::Clamp(1.f - ((ElapsedTime - FadeStart) / (LifeTime - FadeStart)), 0.f, 1.f);

	OnAlphaUpdated(Alpha);
}

void AHitTextActor::SpawnHitText(const UObject* WorldContext, TSubclassOf<AHitTextActor> TextClass,
	const FVector& Location)
{
	// 텍스트 클래스를 안 지정했으면 그냥 넘어감 (호출부에서 매번 검사 안 해도 되게)
	if (!TextClass || !WorldContext)
	{
		return;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return;
	}

	// 기울기에 랜덤을 줘서 매번 조금씩 다르게 보이도록
	AHitTextActor* Defaults = TextClass->GetDefaultObject<AHitTextActor>();
	const float RollRange = Defaults ? Defaults->RandomRollRange : 0.f;
	const FRotator SpawnRotation(0.f, 0.f, FMath::RandRange(-RollRange, RollRange));

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AHitTextActor* HitText = World->SpawnActor<AHitTextActor>(TextClass, Location, SpawnRotation, SpawnParams))
	{
		HitText->InitHitText();
	}
}