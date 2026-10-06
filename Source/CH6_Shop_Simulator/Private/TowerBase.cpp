// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerBase.h"
#include "TowerProjectile.h"
#include "MonsterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "InteractableComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

ATowerBase::ATowerBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 받침대 (고정, 안 돌아감)
	BaseMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComp"));
	RootComponent = BaseMeshComp;

	// 포탑 머리 (몬스터 조준하며 회전, 눈에 안 보이는 순수 피벗) - 받침대 위에 붙임
	TurretHeadComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretHeadComp"));
	TurretHeadComp->SetupAttachment(RootComponent);
	TurretHeadComp->SetRelativeLocation(FVector(0.f, 0.f, 50.f)); // 받침대 위쪽에 배치 (큐브 크기에 맞게 조정)

	// ★ 실제로 보이는 포탑 메시. TurretHeadComp(피벗)의 자식이라 조준 회전은
	// 그대로 따라가지만, 이 컴포넌트 자신의 Relative Rotation은 코드가 안 건드려서
	// 모델링 파일의 정면 축이 안 맞을 때 여기서 수동으로 보정 가능.
	TurretVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretVisualMesh"));
	TurretVisualMesh->SetupAttachment(TurretHeadComp);

	// 탄환이 튀어나올 지점 - 포탑 머리 앞쪽 끝에 배치
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(TurretHeadComp);
	MuzzlePoint->SetRelativeLocation(FVector(50.f, 0.f, 0.f)); // 포탑 머리 큐브 크기에 맞게 조정

	// ★ 스턴 파티클이 나올 자리. 블루프린트에서 눈으로 보며 옮기면 됨.
	// (포탑 머리가 아니라 루트에 붙여야 타워가 조준하며 회전해도 안 흔들림)
	StunEffectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StunEffectPoint"));
	StunEffectPoint->SetupAttachment(RootComponent);
	StunEffectPoint->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
}

void ATowerBase::BeginPlay()
{
	Super::BeginPlay();

	// 감지는 저빈도 타이머로 (매 프레임 스캔하지 않음)
	GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &ATowerBase::ScanForTarget, DetectionInterval, true);

	// ★ 플레이어가 이 타워를 바라볼 때 사거리 원도 같이 보이도록 연동.
	// (InteractableComponent가 붙어있는 타워에만 적용됨 - 없으면 그냥 넘어감)
	if (UInteractableComponent* Interactable = FindComponentByClass<UInteractableComponent>())
	{
		Interactable->OnHighlightChanged.AddDynamic(this, &ATowerBase::HandleHighlightChanged);
	}
}

USceneComponent* ATowerBase::GetActiveMuzzle() const
{
	// 기본은 머즐 포인트 하나. 여러 개를 번갈아 쓰는 타워는 이 함수를 재정의함.
	return MuzzlePoint.Get();
}

void ATowerBase::DisableForDuration(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	const bool bWasAlreadyDisabled = bIsDisabled;
	bIsDisabled = true;

	// ★ 연출은 처음 걸릴 때만 시작. 이미 무력화 중이면 시간만 갱신해서
	// 몽타주가 처음부터 다시 재생되거나 파티클이 중복으로 생기지 않게 함.
	if (!bWasAlreadyDisabled)
	{
		// 스켈레탈 메시가 있는 타워라면 스턴 몽타주 재생
		if (StunMontage)
		{
			// ★ 몽타주가 짧으면 스턴이 끝나기 전에 재생이 먼저 끝나버림.
			// 재생 속도를 조절해서 스턴 지속 시간에 맞춰 늘려줌.
			const float MontageLength = StunMontage->GetPlayLength();
			const float PlayRate = (MontageLength > 0.f) ? (MontageLength / Duration) : 1.f;

			TArray<USkeletalMeshComponent*> SkeletalMeshes;
			GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

			for (USkeletalMeshComponent* Mesh : SkeletalMeshes)
			{
				if (Mesh && Mesh->GetAnimInstance())
				{
					Mesh->GetAnimInstance()->Montage_Play(StunMontage, PlayRate);
				}
			}
		}

		// 머리 위에 스턴 표시 파티클
		if (StunEffect && StunEffectPoint)
		{
			ActiveStunEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
				StunEffect, StunEffectPoint, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset, false);
		}
	}

	GetWorldTimerManager().ClearTimer(DisableTimerHandle);
	GetWorldTimerManager().SetTimer(DisableTimerHandle, this, &ATowerBase::EndDisable, Duration, false);
}

void ATowerBase::EndDisable()
{
	bIsDisabled = false;

	// 스턴 몽타주와 파티클 정리
	if (StunMontage)
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshes;
		GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

		for (USkeletalMeshComponent* Mesh : SkeletalMeshes)
		{
			if (Mesh && Mesh->GetAnimInstance())
			{
				Mesh->GetAnimInstance()->Montage_Stop(0.2f, StunMontage);
			}
		}
	}

	if (ActiveStunEffect)
	{
		ActiveStunEffect->Deactivate();
		ActiveStunEffect = nullptr;
	}
}

void ATowerBase::HandleHighlightChanged(bool bHighlighted)
{
	bShowRangeDebug = bHighlighted;
}

void ATowerBase::ScanForTarget()
{
	// 기존 타겟이 여전히 유효하고 사거리 안이면 계속 그 몬스터를 조준 (매번 다른 놈으로 안 튐)
	if (CurrentTarget && IsValid(CurrentTarget) && CurrentTarget->CurrentState != EMonsterState::Dead)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), CurrentTarget->GetActorLocation());
		if (DistSq <= FMath::Square(Range))
		{
			return;
		}
	}

	// 새 타겟 탐색: 사거리 안에서 가장 가까운 몬스터
	CurrentTarget = nullptr;
	float ClosestDistSq = FMath::Square(Range);

	for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
	{
		AMonsterBase* Monster = *It;
		if (!Monster || Monster->CurrentState == EMonsterState::Dead)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq <= ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			CurrentTarget = Monster;
		}
	}
}

void ATowerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ★ 사거리 시각화. 타겟이 있든 없든 항상 보여야 밸런싱에 도움이 되므로,
	// 아래 early-return보다 먼저 처리함. (수평 원으로 그려서 길목을 얼마나
	// 커버하는지 위에서 내려다보며 확인 가능)
	if (bShowRangeDebug)
	{
		DrawDebugCircle(GetWorld(), GetActorLocation(), Range, 48, FColor::Cyan, false, -1.f, 0, 3.f,
			FVector(1, 0, 0), FVector(0, 1, 0), false);
	}

	// ★ 무력화 중에는 조준 회전도 멈춤. 공격만 막으면 포탑이 계속 목표를
	// 따라 돌아서 무력화된 느낌이 안 남.
	if (bIsDisabled || !CurrentTarget || !IsValid(CurrentTarget))
	{
		return;
	}

	// 포탑 머리를 타겟 방향으로 부드럽게 회전. 좌우(Yaw)만 돌게 하고 상하(Pitch)/기울임(Roll)은
	// 고정해서, 몬스터가 타워보다 높거나 낮은 위치에 있어도 포탑이 위아래로 안 꺾이게 함.
	const FVector ToTarget = CurrentTarget->GetActorLocation() - TurretHeadComp->GetComponentLocation();
	const float TargetYaw = ToTarget.Rotation().Yaw;
	const FRotator CurrentRotation = TurretHeadComp->GetComponentRotation();
	const FRotator DesiredRotation(0.f, TargetYaw, 0.f);
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, RotationInterpSpeed);
	TurretHeadComp->SetWorldRotation(NewRotation);

	// 발사 쿨다운 처리
	FireCooldown -= DeltaTime;
	if (FireCooldown <= 0.f)
	{
		FireAtTarget();
		FireCooldown = FireRate;
	}
}

void ATowerBase::FireAtTarget()
{
	// ★ 보스의 "전쟁의 함성" 등으로 무력화된 상태면 발사하지 않음
	if (bIsDisabled)
	{
		return;
	}

	if (!ProjectileClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			TEXT("⚠ TowerBase: Projectile Class가 지정되지 않았습니다! (블루프린트 디테일 패널 확인)"));
		return;
	}
	if (!CurrentTarget)
	{
		return;
	}

	// ★ 발사 위치는 함수로 빼둠. 궁수 타워처럼 여러 발사 지점을 번갈아 쓰는
	// 경우, 자식 클래스에서 이 함수만 재정의하면 됨.
	USceneComponent* ActiveMuzzle = GetActiveMuzzle();
	if (!ActiveMuzzle)
	{
		return;
	}

	const FVector MuzzleLocation = ActiveMuzzle->GetComponentLocation();

	// ★ 예측 조준(Leading Shot): 몬스터가 이동 중이면, 탄환이 도착할 시점에 몬스터가
	// 있을 위치를 미리 계산해서 그쪽을 조준함. 탄환 속도가 무한대가 아닌 이상,
	// "쏘는 순간의 위치"만 보고 쏘면 이동 중인 몬스터는 항상 뒤쪽을 맞히게 됨.
	const FVector TargetVelocity = CurrentTarget->GetActorForwardVector() * CurrentTarget->GetCurrentSpeed();
	const float DistanceToTarget = FVector::Dist(MuzzleLocation, CurrentTarget->GetActorLocation());
	const float TimeToReach = (ProjectileSpeed > 0.f) ? (DistanceToTarget / ProjectileSpeed) : 0.f;
	const FVector PredictedLocation = CurrentTarget->GetActorLocation() + (TargetVelocity * TimeToReach);

	const FVector Direction = (PredictedLocation - MuzzleLocation).GetSafeNormal();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = this; // 탄환이 스폰 직후 자기를 쏜 타워와 부딪히지 않도록 소유자로 지정

	ATowerProjectile* Projectile = GetWorld()->SpawnActor<ATowerProjectile>(
		ProjectileClass, MuzzleLocation, Direction.Rotation(), SpawnParams);

	if (Projectile)
	{
		if (Projectile->UsesArcTrajectory())
		{
			// ★ 포물선 궤적: 시작점(MuzzleLocation)에서 도착점(PredictedLocation)까지
			// 포물선을 그리며 날아갈 초기 속도를 언리얼이 자동 계산해줌.
			// ArcHeight가 클수록 더 높이 붕 떴다가 떨어지는 궤적이 됨.
			FVector ArcVelocity;
			UGameplayStatics::SuggestProjectileVelocity_CustomArc(
				this, ArcVelocity, MuzzleLocation, PredictedLocation, 0.f, Projectile->GetArcHeight());

			Projectile->InitProjectile(Damage, ArcVelocity);
		}
		else
		{
			// 기존처럼 예측 지점을 향해 일직선으로 발사
			Projectile->InitProjectile(Damage, Direction * ProjectileSpeed);
		}
	}
}

void ATowerBase::ApplyUpgrade()
{
	// 기본 구현: 데미지는 세지고, 공속(발사 간격)은 짧아짐.
	// 사거리 증가나 관통 수 증가처럼 특정 타워에만 있는 특수 효과는
	// 그 타워의 자식 클래스에서 Super::ApplyUpgrade() 호출 후 추가로 처리하면 됨.
	UpgradeTier++;
	Damage *= DamageMultiplierPerTier;
	FireRate *= FireRateMultiplierPerTier;
}