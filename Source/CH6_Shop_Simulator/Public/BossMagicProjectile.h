// Fill out your copyright notice in the Description page of Project Settings.
//
// 보스가 원거리에서 날리는 마법 투사체. 착탄 지점 반경 안의 방어시설과 병사에게
// 범위 데미지를 줌. 캐논 탄환과 비슷하지만 대상이 반대(방어 측)라는 점이 다름.
//
// 몬스터는 통과하도록 콜리전을 설정할 것 (블루프린트에서).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossMagicProjectile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API ABossMagicProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABossMagicProjectile();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	// 착탄 시 데미지가 들어가는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Magic")
	float SplashRadius = 350.f;

	// 날아가는 속도
	UPROPERTY(EditDefaultsOnly, Category = "Magic")
	float Speed = 1200.f;

	// ★ 착탄하는 순간 발생. 블루프린트에서 폭발 이펙트와 사운드를 붙이면 됨.
	// 반경도 같이 넘어가므로 이펙트 크기를 맞출 때 참고할 수 있음.
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic")
	void OnMagicExploded(const FVector& ExplosionLocation, float Radius);

	// 보스가 발사할 때 호출. 목표 방향으로 날려보냄.
	UFUNCTION(BlueprintCallable, Category = "Magic")
	void LaunchTowards(const FVector& TargetLocation, float InDamage);

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	void Explode(const FVector& ImpactLocation);

	float Damage = 30.f;
};