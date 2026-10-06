// Fill out your copyright notice in the Description page of Project Settings.

#include "CannonProjectile.h"
#include "MonsterBase.h"
#include "HitTextActor.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"

ACannonProjectile::ACannonProjectile()
{
	// ★ 대포는 직선이 아니라 포물선으로 날아가야 하니, 중력을 켜고 타워한테
	// "나는 포물선 계산이 필요해"라고 알려줌 (실제 초기 속도 계산은 타워가 함).
	bUseArcTrajectory = true;
	ArcHeight = 0.6f;

	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->ProjectileGravityScale = 1.f;
		ProjectileMovementComp->bRotationFollowsVelocity = true; // 포물선 그리는 동안 탄환 머리가 진행방향을 향하게
	}
}

void ACannonProjectile::Explode()
{
	// 폭발 지점 반경 안의 몬스터를 전부 찾아서 데미지. 폭발은 자주 일어나는
	// 이벤트가 아니라서(초당 몇 발 수준) 전체 순회해도 성능 부담 없음.
	const FVector ExplosionLocation = GetActorLocation();

	// ★ 폭발 연출(이펙트, 사운드, 카메라 흔들림)은 블루프린트에서 이 이벤트를
	// 받아 처리. 폭발 반경을 같이 넘겨주므로 이펙트 크기를 맞출 때 쓸 수 있음.
	OnCannonExploded(ExplosionLocation, SplashRadius);

	// 밸런싱용 범위 표시. 이펙트 크기와 실제 판정이 맞는지 눈으로 확인할 때 사용.
	if (bShowSplashDebug)
	{
		DrawDebugSphere(GetWorld(), ExplosionLocation, SplashRadius, 24, FColor::Orange, false, 1.5f, 0, 2.f);
	}

	// ★ 캐논은 개별 몬스터가 아니라 폭발 지점 위에 텍스트를 한 번만 띄움.
	// (범위 공격이라 맞은 몬스터마다 띄우면 화면이 지저분해짐)
	if (FMath::FRand() <= ExplosionTextChance)
	{
		AHitTextActor::SpawnHitText(this, ExplosionTextClass, ExplosionLocation + HitTextOffset);
	}

	for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
	{
		AMonsterBase* Monster = *It;
		if (!Monster || Monster->CurrentState == EMonsterState::Dead)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(ExplosionLocation, Monster->GetActorLocation());
		if (DistSq <= FMath::Square(SplashRadius))
		{
			FDamageEvent DamageEvent;
			Monster->TakeDamage(Damage, DamageEvent, nullptr, this);
		}
	}

	Destroy();
}

void ACannonProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	// 몬스터는 BeginPlay에서 콜리전 자체를 무시하도록 설정했으므로
	// 여기까지 오는 건 지형이나 구조물뿐임
	Explode();
}

void ACannonProjectile::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	// 몬스터와 겹친 경우는 통과 (범위 폭발로 어차피 데미지가 들어감)
	if (OtherActor && OtherActor->IsA(AMonsterBase::StaticClass()))
	{
		return;
	}

	Explode();
}