// Fill out your copyright notice in the Description page of Project Settings.
//
// 대포 탄환. 맞으면 그 지점 주변 일정 범위 안의 모든 몬스터에게 데미지를 줌 (범위 공격).

#pragma once

#include "CoreMinimal.h"
#include "TowerProjectile.h"
#include "CannonProjectile.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API ACannonProjectile : public ATowerProjectile
{
	GENERATED_BODY()

public:
	ACannonProjectile();

	// 폭발 시 데미지가 들어가는 반경
	UPROPERTY(EditAnywhere, Category = "Cannon")
	float SplashRadius = 300.f;

	// ★ 폭발 반경을 주황색 구체로 표시 (밸런싱용).
	// 이펙트 크기와 실제 판정 범위가 맞는지 눈으로 확인할 때 켜두면 됨.
	UPROPERTY(EditAnywhere, Category = "Cannon|Debug")
	bool bShowSplashDebug = false;

	// ★ 폭발 시 띄울 타격 텍스트 ("KBOOM!" 등). 맞은 몬스터가 아니라
	// 폭발 지점 위에 한 번만 뜸.
	UPROPERTY(EditDefaultsOnly, Category = "Cannon|FX")
	TSubclassOf<class AHitTextActor> ExplosionTextClass;

	// 텍스트가 뜰 확률 (1.0이면 항상, 0.3이면 30%)
	UPROPERTY(EditDefaultsOnly, Category = "Cannon|FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExplosionTextChance = 0.5f;

	// ★ 폭발하는 순간 발생. 블루프린트에서 이 이벤트를 받아 폭발 이펙트와
	// 사운드를 붙이면 됨. 폭발 반경도 같이 넘어가므로 이펙트 크기를 맞출 때
	// 참고할 수 있음 (반경에 비례해 스케일을 키우는 식).
	UFUNCTION(BlueprintImplementableEvent, Category = "Cannon")
	void OnCannonExploded(const FVector& ExplosionLocation, float Radius);

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	void Explode();
};