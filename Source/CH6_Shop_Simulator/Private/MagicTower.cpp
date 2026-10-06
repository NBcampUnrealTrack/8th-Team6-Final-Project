// Fill out your copyright notice in the Description page of Project Settings.

#include "MagicTower.h"
#include "MonsterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

AMagicTower::AMagicTower()
{
	// ★ ArrowTower와 동일한 이유로 RootComponent(고정, 안 도는 받침대)에 붙임.
	// 회전하는 TurretHeadComp에 옆으로 떨어진 위치로 붙이면, 위치 자체가 회전을
	// 따라 빙글빙글 돌아버리는 문제가 있었음.
	MageMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MageMesh"));
	MageMesh->SetupAttachment(RootComponent);
	MageMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f)); // 받침대 중앙에 배치 (크기에 맞게 조정)
}

void AMagicTower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 기존 TurretHeadComp 조준 회전 로직은 그대로 유지

	// ★ 마법사도 위치 고정한 채로, 몬스터 쪽을 바라보는 회전만 매 프레임 계산.
	if (CurrentTarget && IsValid(CurrentTarget))
	{
		const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		if (!ToTarget.IsNearlyZero())
		{
			FRotator FaceRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
			FaceRotation.Yaw += MageFacingYawOffset;

			if (MageMesh)
			{
				MageMesh->SetWorldRotation(FaceRotation);
			}
		}
	}
}

void AMagicTower::FireAtTarget()
{
	// ★ 감지/데미지/투사체 스폰 등 실제 전투 로직은 부모 걸 그대로 재사용.
	// 여기서는 그 위에 시각적인 발사 모션만 얹음.
	Super::FireAtTarget();

	if (CastMontage && MageMesh && MageMesh->GetAnimInstance())
	{
		MageMesh->GetAnimInstance()->Montage_Play(CastMontage);
	}
}