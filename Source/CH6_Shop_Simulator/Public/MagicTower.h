// Fill out your copyright notice in the Description page of Project Settings.
//
// 마법 타워 전용. ATowerBase의 감지/데미지/투사체 로직은 그대로 재사용하고,
// 그 위에 "마법사 1명이 발사 모션을 취하는" 시각 연출만 얹음.
// (ArrowTower와 완전히 동일한 패턴 - 궁수 2명 대신 마법사 1명일 뿐)

#pragma once

#include "CoreMinimal.h"
#include "TowerBase.h"
#include "MagicTower.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;

UCLASS()
class CH6_SHOP_SIMULATOR_API AMagicTower : public ATowerBase
{
	GENERATED_BODY()

public:
	AMagicTower();

protected:
	virtual void FireAtTarget() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MageMesh;

	// 발사할 때 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Mage")
	TObjectPtr<UAnimMontage> CastMontage;

	// 마법사 모델 정면 축 보정 (ArrowTower의 ArcherFacingYawOffset과 동일한 목적)
	UPROPERTY(EditAnywhere, Category = "Mage")
	float MageFacingYawOffset = 0.f;
};