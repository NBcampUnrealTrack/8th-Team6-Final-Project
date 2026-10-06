// Fill out your copyright notice in the Description page of Project Settings.

#include "SoldierUnit.h"
#include "MonsterBase.h"
#include "HitTextActor.h"
#include "DefenseObjectBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/WidgetComponent.h"
#include "HealthBarWidget.h"

ASoldierUnit::ASoldierUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(34.f, 88.f);
	RootComponent = CapsuleComp;

	// ★ 몬스터와는 다른 콜리전 프리셋(에디터에서 미리 만들어둔 "FriendlyUnit").
	// 몬스터는 다른 몬스터(Pawn)는 무시하지만, 새로 만든 이 채널은 명시적으로
	// 무시 설정을 안 해놨기 때문에 기본값(Block)대로 물리적으로 막힘.
	CapsuleComp->SetCollisionProfileName(TEXT("FriendlyUnit"));

	// ★ 프리셋 자체가 "Collision Enabled: No Collision"으로 정의되어 있으면,
	// 채널 응답(Block)을 아무리 맞춰놔도 애초에 충돌 감지 자체가 꺼져있어서 소용없음.
	// 프리셋 정의와 무관하게 코드에서 한 번 더 확실하게 켜둠.
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜리전은 캡슐이 담당

	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComp"));
	HealthBarWidgetComp->SetupAttachment(RootComponent);
	HealthBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComp->SetDrawSize(FVector2D(100.f, 12.f));

	// ★ 스턴 파티클이 나올 자리. 블루프린트에서 눈으로 보며 옮기면 됨.
	StunEffectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StunEffectPoint"));
	StunEffectPoint->SetupAttachment(RootComponent);
	StunEffectPoint->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
}

void ASoldierUnit::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	UpdateHealthBar();

	// 메시 위치도 몬스터랑 똑같이 캡슐 Half Height만큼 내려서 발바닥 맞춤
	if (MeshComp && CapsuleComp)
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleComp->GetScaledCapsuleHalfHeight()));
	}

	// ★ 더 이상 자체적으로 감지 타이머를 돌리지 않음. ASoldierTower가 여러 병사를
	// 보고 중앙에서 SetAssignedTarget()으로 배정해줌.
}

void ASoldierUnit::SetAssignedTarget(AMonsterBase* NewTarget)
{
	CurrentTarget = NewTarget;

	if (!CurrentTarget)
	{
		return;
	}

	// ★ 몬스터의 CurrentState가 "Moving인지"가 아니라, 지금 몬스터가 "실제로
	// 유효한 상대를 물고 있는지"로 판단함. 죽은 병사를 상대로 마지막 몽타주가
	// 재생 중인 동안에는 상태가 여전히 Attacking이지만, 그 상대(몬스터 입장의
	// CurrentTarget)는 이미 파괴되어 무효한 상태 - 이럴 땐 무조건 새로 고정시켜야 함.
	// (안 그러면 병사만 배정된 줄 알고 쫓아다니는데 정작 몬스터는 안 멈추는 버그가 생김)
	ADefenseObjectBase* MonsterEngager = CurrentTarget->CurrentTarget;
	const bool bAlreadyProperlyEngaged = CurrentTarget->CurrentState == EMonsterState::Attacking
		&& IsValid(MonsterEngager);

	if (!bAlreadyProperlyEngaged)
	{
		CurrentTarget->ForceEngage(this);
	}
}

void ASoldierUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ★ 매 프레임 계산할 필요 없이 주기적으로만 플레이어와의 거리를 확인해서
	// 체력바 표시 여부를 결정함.
	HealthBarVisibilityCheckTimer += DeltaTime;
	if (HealthBarVisibilityCheckTimer >= HealthBarVisibilityCheckInterval)
	{
		HealthBarVisibilityCheckTimer = 0.f;

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
				bPlayerInHealthBarRange = (DistSq <= FMath::Square(HealthBarVisibleRange));
			}
		}
		RefreshHealthBarVisibility();
	}

	// ★ 사망 몽타주 재생 중이면 그 무엇도 방해하지 않게 여기서 바로 리턴.
	// (안 그러면 죽어가는 도중에도 원래 싸우던 몬스터가 아직 사거리 안에 있으면
	// 공격 로직이 계속 돌면서 AttackMontage가 DeathMontage를 덮어써버림)
	if (bIsDying)
	{
		return;
	}

	// ★ 마지막 공격 몽타주가 아직 재생 중이면, 새 타겟을 향해 이동하든 원래
	// 자리로 복귀하든 상관없이 일단 그 자리에서 몽타주가 끝날 때까지 대기.
	if (IsWaitingForAttackMontage())
	{
		bIsMovingToTarget = false;
		return;
	}

	const bool bHasValidTarget = CurrentTarget && IsValid(CurrentTarget)
		&& CurrentTarget->CurrentState != EMonsterState::Dead;

	if (!bHasValidTarget)
	{
		CurrentTarget = nullptr; // 확실히 비워서 다음 배정이 새로 들어오게 함
		HandleReturnHome(DeltaTime);
	}
	else
	{
		HandleCombat(DeltaTime);
	}
}

bool ASoldierUnit::IsWaitingForAttackMontage() const
{
	return AttackMontage && MeshComp && MeshComp->GetAnimInstance()
		&& MeshComp->GetAnimInstance()->Montage_IsPlaying(AttackMontage);
}

void ASoldierUnit::HandleReturnHome(float DeltaTime)
{
	// ★ 전투 중이 아닐 때(복귀 중이든 이미 도착해서 대기 중이든) 체력 회복.
	RegenerateHealth(DeltaTime);

	// ★ 상대할 몬스터가 없으면 원래 자리(HomeLocation)로 복귀.
	// (돌아가는 도중에도 타워의 AssignSoldiers는 계속 돌고 있어서, 새 몬스터가
	// 배정되면 바로 다시 ForceEngage로 붙잡혀서 복귀가 자연스럽게 중단됨)
	constexpr float HomeArrivalTolerance = 25.f; // 이 정도 오차면 "도착"으로 침
	const float DistToHome = FVector::Dist(GetActorLocation(), HomeLocation);

	if (DistToHome <= HomeArrivalTolerance)
	{
		// ★ 위치/회전을 정확히 원래 값으로 스냅. 이동 중 누적된 미세한 오차 때문에
		// "도착 판정 안팎을 오가며 계속 Walk 애니메이션이 남는" 문제를 확실히 방지.
		SetActorLocationAndRotation(HomeLocation, HomeRotation);
		bIsMovingToTarget = false; // Idle로
		StuckTimer = 0.f;
		return;
	}

	// ★ 막힘 감지: 지난 프레임과 위치가 거의 안 바뀌었으면(뭔가에 막혀서
	// 실제로 못 움직이는 중) 카운트를 쌓고, 일정 시간 이상 지속되면
	// 그냥 강제로 순간이동시켜서 영원히 Walk 애니메이션에 갇히는 것을 방지.
	const float MovedDistance = FVector::Dist(GetActorLocation(), LastTickLocation);
	StuckTimer = (MovedDistance < 1.f) ? (StuckTimer + DeltaTime) : 0.f;

	constexpr float StuckTimeout = 1.f; // 1초 이상 제자리면 막힌 것으로 판단
	if (StuckTimer >= StuckTimeout)
	{
		SetActorLocationAndRotation(HomeLocation, HomeRotation);
		bIsMovingToTarget = false;
		StuckTimer = 0.f;
		LastTickLocation = GetActorLocation();
		return;
	}

	bIsMovingToTarget = true; // 복귀 중에도 걷는 애니메이션 유지
	const FVector Direction = (HomeLocation - GetActorLocation()).GetSafeNormal();
	FVector NewLocation = GetActorLocation() + (Direction * MoveSpeed * DeltaTime);

	// 이번 프레임 이동량이 남은 거리보다 크면(도착 직전 마지막 스텝), 그냥
	// 정확히 HomeLocation으로 스냅. 안 그러면 목표를 살짝 지나쳤다가 되돌아오길
	// 반복하면서 도착 판정이 계속 안 나는 미세한 진동이 생길 수 있음.
	if (FVector::Dist(NewLocation, HomeLocation) > DistToHome)
	{
		NewLocation = HomeLocation;
	}

	LastTickLocation = GetActorLocation();
	SetActorLocation(NewLocation, true);

	// 회전은 수평만 (위아래로 몸이 기울어지지 않게)
	FVector FlatDirection = Direction;
	FlatDirection.Z = 0.f;
	if (!FlatDirection.IsNearlyZero())
	{
		SetActorRotation(FlatDirection.GetSafeNormal().Rotation());
	}
}

void ASoldierUnit::DisableForDuration(float Duration)
{
	// 죽어가는 중이면 무의미하므로 무시
	if (Duration <= 0.f || bIsDying)
	{
		return;
	}

	const bool bWasAlreadyDisabled = bIsDisabled;
	bIsDisabled = true;
	bIsMovingToTarget = false;

	// ★ 연출은 처음 걸릴 때만 시작. 이미 무력화 중이면 시간만 갱신해서
	// 몽타주가 처음부터 다시 재생되거나 파티클이 중복으로 생기지 않게 함.
	if (!bWasAlreadyDisabled)
	{
		if (StunMontage && MeshComp && MeshComp->GetAnimInstance())
		{
			// ★ 몽타주가 짧으면 스턴이 끝나기 전에 재생이 먼저 끝나버림.
			// 재생 속도를 조절해서 스턴 지속 시간에 맞춰 늘려줌.
			const float MontageLength = StunMontage->GetPlayLength();
			const float PlayRate = (MontageLength > 0.f) ? (MontageLength / Duration) : 1.f;

			MeshComp->GetAnimInstance()->Montage_Play(StunMontage, PlayRate);
		}

		if (StunEffect && StunEffectPoint)
		{
			ActiveStunEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
				StunEffect, StunEffectPoint, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset, false);
		}
	}

	GetWorldTimerManager().ClearTimer(DisableTimerHandle);
	GetWorldTimerManager().SetTimer(DisableTimerHandle, this, &ASoldierUnit::EndDisable, Duration, false);
}

void ASoldierUnit::EndDisable()
{
	bIsDisabled = false;

	if (StunMontage && MeshComp && MeshComp->GetAnimInstance())
	{
		MeshComp->GetAnimInstance()->Montage_Stop(0.2f, StunMontage);
	}

	if (ActiveStunEffect)
	{
		ActiveStunEffect->Deactivate();
		ActiveStunEffect = nullptr;
	}
}

void ASoldierUnit::HandleCombat(float DeltaTime)
{
	// ★ 무력화 상태면 제자리에 멈춰서 아무것도 안 함 (보스의 전쟁의 함성 등)
	if (bIsDisabled)
	{
		bIsMovingToTarget = false;
		return;
	}

	// ★ ForceEngage로 몬스터가 이미 멈춰있는 상태라서, 더 이상 "앞쪽을 예측"할
	// 필요 없이 그냥 몬스터의 현재(고정된) 위치로 다가가면 됨.
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const float DistSq = FVector::DistSquared(GetActorLocation(), TargetLocation);

	if (DistSq > FMath::Square(MeleeRange))
	{
		// ★ 아직 근접 사거리 밖 -> 몬스터에게 다가감.
		// bSweep=true라서 벽이나 다른 유닛에는 물리적으로 막힘 (몬스터에게 닿으면
		// 그 자리에서 자연스럽게 멈추게 됨 - 콜리전이 그 역할을 대신 해줌).
		bIsMovingToTarget = true; // AnimBP가 이 값을 보고 Walk 애니메이션으로 전환

		// ★ Z를 제거해서 수평으로만 이동. 보스처럼 덩치가 큰 몬스터는 액터 중심이
		// 병사보다 훨씬 위에 있어서, 그대로 방향을 잡으면 병사가 공중으로
		// 떠오르며 다가가는 문제가 있었음.
		FVector Direction = TargetLocation - GetActorLocation();
		Direction.Z = 0.f;
		Direction = Direction.GetSafeNormal();

		// ★ 높이는 현재 위치 그대로 유지. bSweep으로 이동하므로 수평 성분만
		// 더해주면 바닥에 스치면서 막히는 일 없이 자연스럽게 걸어감.
		FVector NewLocation = GetActorLocation() + (Direction * MoveSpeed * DeltaTime);
		NewLocation.Z = GetActorLocation().Z;

		SetActorLocation(NewLocation, true);
		SetActorRotation(Direction.Rotation());
		return;
	}

	// 근접 사거리 안 -> 멈춰서 공격 (몬스터 쪽을 바라보게 회전)
	bIsMovingToTarget = false; // 멈췄으니 Idle/Attack 쪽으로

	// 바라보는 방향도 수평만. 안 그러면 큰 몬스터를 올려다보며 몸이 기울어짐.
	FVector LookDirection = CurrentTarget->GetActorLocation() - GetActorLocation();
	LookDirection.Z = 0.f;
	SetActorRotation(LookDirection.GetSafeNormal().Rotation());

	AttackTimer += DeltaTime;
	if (AttackTimer >= AttackInterval)
	{
		AttackTimer = 0.f;

		if (AttackMontage && MeshComp && MeshComp->GetAnimInstance())
		{
			MeshComp->GetAnimInstance()->Montage_Play(AttackMontage);
		}

		FDamageEvent DamageEvent;
		CurrentTarget->TakeDamage(AttackDamage, DamageEvent, nullptr, this);

		// ★ 확률적으로 타격 텍스트 ("POW!", "SOK!" 등). 매번 뜨면 화면이
		// 지저분해지므로 가끔씩만 뜨게 해서 리듬감을 줌.
		if (FMath::FRand() <= HitTextChance)
		{
			AHitTextActor::SpawnHitText(this, HitTextClass,
				CurrentTarget->GetActorLocation() + HitTextOffset);
		}
	}
}

void ASoldierUnit::RegenerateHealth(float DeltaTime)
{
	if (CurrentHealth >= MaxHealth)
	{
		return; // 이미 풀피면 할 일 없음
	}

	// CurrentHealth가 정수(int32)라서, 소수점 회복량이 매 프레임 그냥 버려지지
	// 않도록 누적해뒀다가 1 이상 쌓이면 그만큼만 정수로 반영함.
	RegenAccumulator += HealthRegenPerSecond * DeltaTime;
	if (RegenAccumulator >= 1.f)
	{
		const int32 HealAmount = FMath::FloorToInt(RegenAccumulator);
		CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
		RegenAccumulator -= HealAmount;
		UpdateHealthBar();
	}
}

void ASoldierUnit::UpdateHealthBar()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	if (UHealthBarWidget* Widget = Cast<UHealthBarWidget>(HealthBarWidgetComp->GetUserWidgetObject()))
	{
		const float Percent = (MaxHealth > 0) ? (static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)) : 0.f;
		Widget->UpdateHealth(Percent);
	}

	RefreshHealthBarVisibility();
}

void ASoldierUnit::RefreshHealthBarVisibility()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	const bool bShouldShow = bPlayerInHealthBarRange && (CurrentHealth < MaxHealth) && !bIsDying;
	HealthBarWidgetComp->SetVisibility(bShouldShow);
}

float ASoldierUnit::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentHealth <= 0)
	{
		return 0.f; // 이미 죽어가는 중 (몽타주 재생 중)
	}

	CurrentHealth -= FMath::RoundToInt(DamageAmount);
	UpdateHealthBar();

	if (CurrentHealth <= 0)
	{
		Die();
	}

	return DamageAmount;
}

void ASoldierUnit::Die()
{
	bIsDying = true; // Tick의 이동/전투 로직 전부 정지

	// 죽으면 콜리전 꺼서 몽타주 재생 중 다른 몬스터/병사가 시체에 부딪히는 걸 방지
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DeathMontage && MeshComp && MeshComp->GetAnimInstance())
	{
		const float MontageDuration = MeshComp->GetAnimInstance()->Montage_Play(DeathMontage);
		if (MontageDuration > 0.f)
		{
			const float DestroyDelay = FMath::Max(0.f, MontageDuration - DeathMontageEndBuffer);
			GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &ASoldierUnit::FinishDying, DestroyDelay, false);
			return;
		}
	}

	// 몽타주가 없거나 재생 실패하면 그냥 바로 파괴
	FinishDying();
}

void ASoldierUnit::FinishDying()
{
	Destroy();
}