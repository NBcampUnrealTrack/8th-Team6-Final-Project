// Fill out your copyright notice in the Description page of Project Settings.
//
// 애로우 타워 전용. 기존 ATowerBase의 감지/데미지/투사체 로직은 그대로 재사용하고,
// 그 위에 "궁수 2명이 번갈아 쏘는 모션"만 시각적으로 얹음. 두 궁수 메시는
// TurretHeadComp(조준 피벗)의 자식이라, 몬스터 조준 방향을 자동으로 따라감.

#pragma once

#include "CoreMinimal.h"
#include "TowerBase.h"
#include "ArrowTower.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;
class USceneComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API AArrowTower : public ATowerBase
{
	GENERATED_BODY()

public:
	AArrowTower();

protected:
	// 부모의 발사 로직(감지/데미지/투사체)은 그대로 실행하고, 그 위에 궁수
	// 발사 모션만 추가로 재생함.
	virtual void FireAtTarget() override;

	// ★ 궁수들이 위치 고정된 채로 몸통만 몬스터 쪽으로 돌게 직접 회전 계산
	virtual void Tick(float DeltaTime) override;

	// 지금 쏠 차례인 궁수의 발사 위치를 반환
	virtual USceneComponent* GetActiveMuzzle() const override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ArcherMesh1;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ArcherMesh2;

	// ★ 궁수마다 각자의 발사 위치. 각 궁수 메시의 자식이라 궁수가 몬스터를
	// 향해 회전하면 같이 돌아감. (부모의 MuzzlePoint는 TurretHeadComp에 붙어
	// 있어서 궁수 회전을 안 따라가, 화살이 엉뚱한 데서 나오는 문제가 있었음)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> ArcherMuzzle1;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> ArcherMuzzle2;

	// 발사할 때 재생할 몽타주 (궁수 애셋에 맞는 것 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Archer")
	TObjectPtr<UAnimMontage> ShootMontage;

	// ★ 궁수 모델 자체의 "정면" 축이 언리얼 기준과 다르게 만들어져 있을 때 보정.
	// 계산된 조준 방향은 정확한데 모델이 엉뚱한 곳을 보는 것처럼 보인다면 이 값을
	// 조정 (예: 정확히 뒤를 보고 있다면 180을 넣으면 됨).
	UPROPERTY(EditAnywhere, Category = "Archer")
	float ArcherFacingYawOffset = 0.f;

private:
	// 매번 다른 궁수가 쏘도록 번갈아가며 표시 (둘이 동시에 쏘면 어색하니까)
	bool bNextIsArcher1 = true;
};