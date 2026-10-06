// Fill out your copyright notice in the Description page of Project Settings.

#include "BossAxeProjectile.h"
#include "TrainPawn.h"
#include "MonsterBase.h"
#include "SplineFollowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"

ABossAxeProjectile::ABossAxeProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetMobility(EComponentMobility::Movable);
	// ★ 콜리전 프리셋은 여기서 강제하지 않고 블루프린트에서 지정하도록 둠.
	// (Floor와 Pawn에만 Block하는 커스텀 프리셋 등을 쓸 수 있게)
	MeshComp->OnComponentHit.AddDynamic(this, &ABossAxeProjectile::HandleHit);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->SetUpdatedComponent(MeshComp);
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bShouldBounce = false;
	ProjectileMovementComp->ProjectileGravityScale = 1.f; // 포물선을 그리려면 중력 필요

	InitialLifeSpan = 10.f; // 어딘가에 끼어서 영원히 남지 않도록
}

void ABossAxeProjectile::LifeSpanExpired()
{
	// ★ 아무것도 안 맞고 수명이 다한 경우에도 그 자리에서 착탄 처리.
	// (콜리전 설정이 안 맞아 통과해버려도 최소한 효과는 나가게)
	Explode(GetActorLocation());
}

void ABossAxeProjectile::LaunchAt(const FVector& TargetLocation)
{
	CachedTargetLocation = TargetLocation;
	bHasLaunched = true;

	// ★ 던진 보스 본인과는 충돌하지 않게 함. 보스 몸집이 크면 도끼가
	// 보스 안에서 생성되어 즉시 터져버리기 때문.
	if (AActor* MyOwner = GetOwner())
	{
		MeshComp->IgnoreActorWhenMoving(MyOwner, true);
	}

	// ★ 포물선 속도를 직접 계산함. 엔진의 SuggestProjectileVelocity_CustomArc는
	// 여러 해 중 하나를 골라주는 방식이라 예상과 다른 궤적이 나올 때가 있어서,
	// "정해진 시간(FlightTime) 안에 목표에 도달하는 속도"를 물리 공식으로 직접 구함.
	//
	// 수평: 거리 / 시간
	// 수직: (높이차 + 0.5 * 중력 * 시간^2) / 시간
	const FVector Start = GetActorLocation();
	const FVector Delta = TargetLocation - Start;

	const float SafeTime = FMath::Max(0.1f, FlightTime); // 0으로 나누기 방지
	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ()) * ProjectileMovementComp->ProjectileGravityScale;

	FVector LaunchVelocity;
	LaunchVelocity.X = Delta.X / SafeTime;
	LaunchVelocity.Y = Delta.Y / SafeTime;
	LaunchVelocity.Z = (Delta.Z + 0.5f * Gravity * SafeTime * SafeTime) / SafeTime;

	ProjectileMovementComp->Velocity = LaunchVelocity;

	// ★ 계산한 비행 시간이 지나면 충돌 여부와 상관없이 착탄 처리.
	// 콜리전 설정이 안 맞아 기차를 통과해버려도 목표 지점에서 확실히 터지게 함.
	GetWorldTimerManager().SetTimer(ImpactTimerHandle, this,
		&ABossAxeProjectile::HandleFlightTimeEnd, SafeTime, false);

	OnAxeLaunched(TargetLocation);
}

void ABossAxeProjectile::HandleFlightTimeEnd()
{
	// 예정된 착탄 지점에서 터뜨림 (실제 도끼 위치는 오차가 있을 수 있으므로
	// 경고 원을 그렸던 지점을 기준으로 함 - 플레이어가 본 것과 일치시킴)
	Explode(CachedTargetLocation);
}

void ABossAxeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasLaunched)
	{
		return;
	}

	// ★ 날아가는 동안 착탄 예정 지점을 바닥에 표시해서 플레이어가 피할 수 있게 함.
	// (나중에 데칼이나 나이아가라 이펙트로 교체하면 더 보기 좋음)
	if (bShowWarningCircle)
	{
		DrawDebugCircle(GetWorld(), CachedTargetLocation, ImpactRadius, 32, FColor::Red,
			false, -1.f, 0, 5.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
}

void ABossAxeProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 던진 보스 본인과 부딪힌 거면 무시 (생성 직후 자기 몸에 닿는 경우)
	if (OtherActor == GetOwner())
	{
		return;
	}

	// ★ 몬스터는 통과함. 도끼는 기차를 노리는 공격인데, 경로에 몬스터가
	// 지나가면 거기서 터져버려서 정작 기차에 닿지 못하는 문제가 있었음.
	// 어차피 비행 시간이 끝나면 목표 지점에서 자동으로 터지므로 그냥 무시.
	if (OtherActor && OtherActor->IsA(AMonsterBase::StaticClass()))
	{
		MeshComp->IgnoreActorWhenMoving(OtherActor, true);
		return;
	}

	Explode(Hit.ImpactPoint);
}

void ABossAxeProjectile::Explode(const FVector& ImpactLocation)
{
	OnAxeImpact(ImpactLocation);

	// 착탄 반경 안에 기차가 있으면 레일 위에서 뒤로 밀어냄
	for (TActorIterator<ATrainPawn> It(GetWorld()); It; ++It)
	{
		ATrainPawn* Train = *It;
		if (!Train || !Train->SplineFollowComp)
		{
			continue;
		}

		if (FVector::Dist(ImpactLocation, Train->GetActorLocation()) > ImpactRadius)
		{
			continue;
		}

		// ★ 기차는 레일 위를 스플라인 거리로 움직이므로, 밀어내기는
		// "지금 거리에서 뒤로 빼기"로 처리함. 한 번에 빼면 순간이동처럼
		// 보이므로, 기차 쪽에서 여러 프레임에 걸쳐 부드럽게 밀려나게 함.
		Train->ApplyKnockback(KnockbackDistance, KnockbackDuration);

		// 명중 순간 카메라 흔들림 (기차에 탑승 중인 플레이어가 체감하도록)
		if (HitCameraShake)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraShake(HitCameraShake);
				}
			}
		}
	}

	Destroy();
}