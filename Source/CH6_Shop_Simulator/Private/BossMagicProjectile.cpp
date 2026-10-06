// Fill out your copyright notice in the Description page of Project Settings.

#include "BossMagicProjectile.h"
#include "DefenseObjectBase.h"
#include "MonsterBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"

ABossMagicProjectile::ABossMagicProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetMobility(EComponentMobility::Movable);
	// 콜리전 프리셋은 블루프린트에서 지정할 것.
	// 몬스터는 통과해야 하므로 몬스터 채널은 Ignore로 설정.
	MeshComp->OnComponentHit.AddDynamic(this, &ABossMagicProjectile::HandleHit);

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->SetUpdatedComponent(MeshComp);
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bShouldBounce = false;
	ProjectileMovementComp->ProjectileGravityScale = 0.f; // 마법구는 직선으로 날아감

	InitialLifeSpan = 6.f; // 어딘가에 끼어서 영원히 남지 않도록
}

void ABossMagicProjectile::LaunchTowards(const FVector& TargetLocation, float InDamage)
{
	Damage = InDamage;

	const FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
	ProjectileMovementComp->Velocity = Direction * Speed;

	// 발사한 보스와는 충돌하지 않게
	if (AActor* MyOwner = GetOwner())
	{
		MeshComp->IgnoreActorWhenMoving(MyOwner, true);
	}
}

void ABossMagicProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	// ★ 방어시설(바리케이드, 병사, 코어)에 맞으면 그 자리에서 터짐
	if (OtherActor && OtherActor->IsA(ADefenseObjectBase::StaticClass()))
	{
		Explode(Hit.ImpactPoint);
		return;
	}

	// ★ 몬스터와 플레이어는 통과. 이 마법구는 방어시설만 노리는 공격이라,
	// 중간에 아군 몬스터나 플레이어에 맞고 터지면 안 됨.
	if (OtherActor && (OtherActor->IsA(AMonsterBase::StaticClass()) || OtherActor->IsA(APawn::StaticClass())))
	{
		MeshComp->IgnoreActorWhenMoving(OtherActor, true);
		return;
	}

	// 그 외(지형, 구조물)에 맞으면 터짐
	Explode(Hit.ImpactPoint);
}

void ABossMagicProjectile::Explode(const FVector& ImpactLocation)
{
	// 폭발 연출은 블루프린트에서 이 이벤트를 받아 처리
	OnMagicExploded(ImpactLocation, SplashRadius);

	// ★ 반경 안의 방어시설과 병사에게 범위 데미지.
	// ADefenseObjectBase를 상속받은 것들(바리케이드, 병사, 코어)이 대상.
	for (TActorIterator<ADefenseObjectBase> It(GetWorld()); It; ++It)
	{
		ADefenseObjectBase* Target = *It;
		if (!Target)
		{
			continue;
		}

		if (FVector::DistSquared(ImpactLocation, Target->GetActorLocation()) <= FMath::Square(SplashRadius))
		{
			FDamageEvent DamageEvent;
			Target->TakeDamage(Damage, DamageEvent, nullptr, GetOwner());
		}
	}

	Destroy();
}