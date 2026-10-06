// Fill out your copyright notice in the Description page of Project Settings.

#include "ArrowTower.h"
#include "MonsterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

AArrowTower::AArrowTower()
{
	// ★ TurretHeadComp(회전하는 피벗)가 아니라 RootComponent(고정, 안 도는 받침대)에
	// 붙임. 회전하는 부모에 옆으로 떨어진 위치로 붙이면, 그 위치 자체가 회전을
	// 따라 빙글빙글 돌아버리는 문제가 있었음. 위치는 고정하고, 회전(몸통 방향)은
	// Tick에서 직접 계산해서 따로 처리함.
	ArcherMesh1 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArcherMesh1"));
	ArcherMesh1->SetupAttachment(RootComponent);
	ArcherMesh1->SetRelativeLocation(FVector(0.f, -30.f, 0.f)); // 좌측에 배치 (크기에 맞게 조정)

	ArcherMesh2 = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArcherMesh2"));
	ArcherMesh2->SetupAttachment(RootComponent);
	ArcherMesh2->SetRelativeLocation(FVector(0.f, 30.f, 0.f)); // 우측에 배치

	// ★ 각 궁수의 발사 위치. 궁수 메시의 자식이라 궁수가 회전하면 같이 돌아감.
	// 뷰포트에서 활시위 쪽으로 옮겨두면 화살이 그 자리에서 나감.
	ArcherMuzzle1 = CreateDefaultSubobject<USceneComponent>(TEXT("ArcherMuzzle1"));
	ArcherMuzzle1->SetupAttachment(ArcherMesh1);
	ArcherMuzzle1->SetRelativeLocation(FVector(50.f, 0.f, 100.f));

	ArcherMuzzle2 = CreateDefaultSubobject<USceneComponent>(TEXT("ArcherMuzzle2"));
	ArcherMuzzle2->SetupAttachment(ArcherMesh2);
	ArcherMuzzle2->SetRelativeLocation(FVector(50.f, 0.f, 100.f));
}

USceneComponent* AArrowTower::GetActiveMuzzle() const
{
	// 지금 쏠 차례인 궁수의 발사 위치를 돌려줌.
	// (차례를 넘기는 건 FireAtTarget에서 부모 호출이 끝난 뒤에 함)
	USceneComponent* Muzzle = bNextIsArcher1 ? ArcherMuzzle1.Get() : ArcherMuzzle2.Get();
	return Muzzle ? Muzzle : MuzzlePoint.Get(); // 못 찾으면 부모 기본값으로 대체
}

void AArrowTower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 기존 TurretHeadComp 조준 회전 로직은 그대로 유지

	// ★ 궁수 몸통은 위치 고정한 채로, 몬스터 쪽을 바라보는 회전만 매 프레임 계산.
	// Yaw만 반영해서 위아래로 안 기울어지게 함 (몬스터/병사 회전 때와 동일한 패턴).
	if (CurrentTarget && IsValid(CurrentTarget))
	{
		const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		if (!ToTarget.IsNearlyZero())
		{
			FRotator FaceRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
			FaceRotation.Yaw += ArcherFacingYawOffset; // 모델 정면 축 보정

			if (ArcherMesh1)
			{
				ArcherMesh1->SetWorldRotation(FaceRotation);
			}
			if (ArcherMesh2)
			{
				ArcherMesh2->SetWorldRotation(FaceRotation);
			}
		}
	}
}

void AArrowTower::FireAtTarget()
{
	// ★ 감지/데미지/투사체 스폰 등 실제 전투 로직은 부모 걸 그대로 재사용.
	// 부모가 GetActiveMuzzle()을 호출해서 지금 차례인 궁수 위치에서 화살을 쏨.
	Super::FireAtTarget();

	// 방금 쏜 궁수에게 발사 모션을 재생시키고, 다음 차례를 넘김.
	// (차례를 여기서 넘겨야 위 Super 호출 때 올바른 궁수 위치가 쓰임)
	USkeletalMeshComponent* Shooter = bNextIsArcher1 ? ArcherMesh1 : ArcherMesh2;
	bNextIsArcher1 = !bNextIsArcher1;

	if (ShootMontage && Shooter && Shooter->GetAnimInstance())
	{
		Shooter->GetAnimInstance()->Montage_Play(ShootMontage);
	}
}