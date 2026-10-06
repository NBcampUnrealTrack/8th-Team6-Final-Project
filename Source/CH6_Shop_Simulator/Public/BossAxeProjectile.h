// Fill out your copyright notice in the Description page of Project Settings.
//
// 오크 족장이 기차를 향해 던지는 도끼. 포물선으로 날아가서 착탄하면
// 그 반경 안의 기차를 레일 위에서 뒤로 밀어냄.
//
// 던지는 순간부터 착탄까지 바닥에 경고 원이 표시되어, 플레이어가 기차를
// 움직여 피할 수 있게 함.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossAxeProjectile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API ABossAxeProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABossAxeProjectile();

	virtual void Tick(float DeltaTime) override;

	// 아무것도 안 맞고 수명이 다한 경우에도 그 자리에서 착탄 처리
	virtual void LifeSpanExpired() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	// 착탄 시 이 반경 안에 있는 기차를 밀어냄
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	float ImpactRadius = 500.f;

	// 레일 위에서 뒤로 밀려나는 거리 (너무 크면 짧은 지름길 레일에서
	// 순식간에 시작점까지 밀려나므로 적당히)
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	float KnockbackDistance = 400.f;

	// 밀려나는 데 걸리는 시간(초). 짧으면 급격하게, 길면 부드럽게 밀려남.
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	float KnockbackDuration = 0.8f;

	// ★ 던져서 착탄할 때까지 걸리는 시간(초). 이 값이 곧 플레이어가 피할 수
	// 있는 시간이므로 밸런싱의 핵심. 길면 피하기 쉽고 짧으면 어려움.
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	float FlightTime = 2.f;

	// 착탄 경고 원을 표시할지 (나중에 데칼/이펙트로 교체 가능)
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	bool bShowWarningCircle = true;

	// 기차에 명중했을 때 재생할 카메라 흔들림
	UPROPERTY(EditDefaultsOnly, Category = "Axe")
	TSubclassOf<class UCameraShakeBase> HitCameraShake;

	// 보스가 던질 때 호출: 목표 지점을 향해 포물선으로 발사
	UFUNCTION(BlueprintCallable, Category = "Axe")
	void LaunchAt(const FVector& TargetLocation);

	// ★ 착탄 순간 발생. 블루프린트에서 이펙트나 사운드를 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Axe")
	void OnAxeImpact(const FVector& ImpactLocation);

	// ★ 던져진 순간 발생. 경고음 등을 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Axe")
	void OnAxeLaunched(const FVector& PredictedImpactLocation);

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// 비행 시간이 다 되면 호출됨 (충돌을 못 해도 확실히 착탄시키기 위함)
	void HandleFlightTimeEnd();

	void Explode(const FVector& ImpactLocation);

	FTimerHandle ImpactTimerHandle;

	// 던질 때 계산한 예상 착탄 지점 (경고 원 표시용)
	FVector CachedTargetLocation = FVector::ZeroVector;
	bool bHasLaunched = false;
};