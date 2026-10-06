// Fill out your copyright notice in the Description page of Project Settings.

#include "BossMonsterBase.h"
#include "DefenseObjectBase.h"
#include "DefenseWall.h"
#include "SplineFollowComponent.h"
#include "BossAxeProjectile.h"
#include "BossHealthBarWidget.h"
#include "BossMagicProjectile.h"
#include "TrainPawn.h"
#include "TowerBase.h"
#include "SoldierUnit.h"
#include "SoldierTower.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

ABossMonsterBase::ABossMonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// ★ 함성 이펙트가 나올 자리. 블루프린트에서 눈으로 보며 옮기면 됨.
	WarCryEffectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WarCryEffectPoint"));
	WarCryEffectPoint->SetupAttachment(RootComponent);

	// ★ 아우라 이펙트가 나올 자리 (보통 발밑)
	AuraEffectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AuraEffectPoint"));
	AuraEffectPoint->SetupAttachment(RootComponent);
}

void ABossMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	// 소환 기능을 켠 보스만 타이머를 돌림
	if (bUseSummon && SummonClass)
	{
		GetWorldTimerManager().SetTimer(SummonTimerHandle, this,
			&ABossMonsterBase::SummonMinions, SummonInterval, true);
	}

	// 도끼 던지기도 마찬가지
	if (bUseAxeThrow && AxeClass)
	{
		GetWorldTimerManager().SetTimer(AxeThrowTimerHandle, this,
			&ABossMonsterBase::ThrowAxe, AxeThrowInterval, true);
	}

	// 궁극기(전쟁의 함성)
	if (bUseWarCry)
	{
		GetWorldTimerManager().SetTimer(WarCryTimerHandle, this,
			&ABossMonsterBase::TriggerWarCry, WarCryInterval, true);
	}

	// 원거리 포격
	if (bUseRangedAttack && MagicProjectileClass)
	{
		GetWorldTimerManager().SetTimer(RangedAttackTimerHandle, this,
			&ABossMonsterBase::FireRangedAttack, RangedAttackInterval, true);
	}

	// ★ 아우라 버프를 쓰는 보스는 발밑에 이펙트를 계속 띄움.
	// 이펙트를 직접 수정하지 않고, 버프 반경에 맞춰 크기만 코드로 늘려줌.
	if (bUseAuraBuff && AuraEffect && AuraEffectPoint)
	{
		ActiveAuraEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
			AuraEffect, AuraEffectPoint, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, false);

		if (ActiveAuraEffect && AuraEffectBaseRadius > 0.f)
		{
			// 원래 크기가 덮는 반경 대비 몇 배로 키워야 하는지 계산.
			// 보스 자체 스케일도 곱해지므로 그만큼 나눠서 상쇄함.
			const float ActorScale = GetActorScale3D().X;
			const float SafeActorScale = FMath::IsNearlyZero(ActorScale) ? 1.f : ActorScale;
			const float EffectScale = (BuffRadius / AuraEffectBaseRadius) / SafeActorScale;

			ActiveAuraEffect->SetRelativeScale3D(FVector(EffectScale));
		}
	}

	// ★ 화면 상단에 보스 체력바를 띄움. 보스가 등장했다는 걸 알리는 역할도 함.
	if (BossHealthBarClass)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			BossHealthBarInstance = CreateWidget<UBossHealthBarWidget>(PC, BossHealthBarClass);
			if (BossHealthBarInstance)
			{
				BossHealthBarInstance->AddToViewport();
				BossHealthBarInstance->ShowForBoss(BossDisplayName);
			}
		}
	}
}

float ABossMonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (BossHealthBarInstance && MaxHealth > 0.f)
	{
		BossHealthBarInstance->UpdateHealth(CurrentHealth / MaxHealth);
		BossHealthBarInstance->PlayHitShake();

		// 죽었으면 체력바를 치움
		if (CurrentHealth <= 0.f)
		{
			BossHealthBarInstance->HideBar();
		}
	}

	return ActualDamage;
}

void ABossMonsterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 시전 중 상태로 남지 않도록 정리
	bIsCasting = false;
	bIsCastingSkill = false;

	// 아우라 이펙트 정리
	if (ActiveAuraEffect)
	{
		ActiveAuraEffect->Deactivate();
		ActiveAuraEffect = nullptr;
	}

	// 보스가 사라질 때 체력바도 같이 정리 (레벨 이동 등으로 남아있지 않도록)
	if (BossHealthBarInstance)
	{
		BossHealthBarInstance->RemoveFromParent();
		BossHealthBarInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ABossMonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 죽었으면 스킬도 멈춤
	if (CurrentState == EMonsterState::Dead)
	{
		return;
	}

	// ★ 원거리 보스는 사거리 안에 방어 대상이 들어오면 멈춰서 포격함.
	// (계속 전진하면서 쏘면 결국 근접까지 붙어버려서 원거리 컨셉이 무의미해짐)
	// 시전 중(bIsCastingSkill)일 때는 그 상태를 유지해야 하므로 덮어쓰지 않음.
	if (bUseRangedAttack && !bIsCastingSkill)
	{
		bIsCasting = (FindRangedTarget() != nullptr);
	}

	// 버프는 매 프레임 갱신할 필요가 없어서 주기적으로만 처리
	if (bUseAuraBuff)
	{
		BuffTimer += DeltaTime;
		if (BuffTimer >= BuffUpdateInterval)
		{
			BuffTimer = 0.f;
			UpdateAuraBuff();
		}
	}

	if (bShowRangeDebug)
	{
		if (bUseSplashAttack)
		{
			DrawDebugCircle(GetWorld(), GetActorLocation(), SplashRadius, 32, FColor::Red,
				false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		}
		if (bUseAuraBuff)
		{
			DrawDebugCircle(GetWorld(), GetActorLocation(), BuffRadius, 32, FColor::Purple,
				false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		}
	}
}

void ABossMonsterBase::PerformAttack()
{
	// 주 대상은 부모 방식 그대로 풀 데미지
	Super::PerformAttack();

	if (!bUseSplashAttack || !CurrentTarget)
	{
		return;
	}

	// ★ 주 대상 주변의 다른 방어시설/병사도 같이 타격.
	// 병사들이 보스를 둘러싸고 다구리 치는 상황에서 보스가 한 명씩만
	// 상대하면 너무 쉬워지므로, 광역으로 한꺼번에 밀어내게 함.
	const FVector Center = CurrentTarget->GetActorLocation();
	const float SplashDamage = AttackDamage * SplashDamageRatio;

	// 충격파 연출은 블루프린트에서 이 이벤트를 받아 처리
	OnSplashAttack(Center, SplashRadius);

	for (TActorIterator<ADefenseObjectBase> It(GetWorld()); It; ++It)
	{
		ADefenseObjectBase* Target = *It;
		if (!Target || Target == CurrentTarget)
		{
			continue; // 주 대상은 이미 위에서 때렸음
		}

		if (FVector::DistSquared(Center, Target->GetActorLocation()) <= FMath::Square(SplashRadius))
		{
			FDamageEvent DamageEvent;
			Target->TakeDamage(SplashDamage, DamageEvent, nullptr, this);
		}
	}
}

void ABossMonsterBase::SummonMinions()
{
	if (!SummonClass || CurrentState == EMonsterState::Dead || !GetWorld())
	{
		return;
	}

	// 죽은 부하 기록을 먼저 정리 (목록이 무한정 쌓이는 것 방지)
	SummonedMinions.RemoveAll([](const TObjectPtr<AMonsterBase>& Minion)
		{
			return !IsValid(Minion);
		});

	// ★ 상한을 넘으면 더 소환하지 않음. 보스가 오래 버틸수록 몬스터가 무한정
	// 쌓여서 프레임이 무너지는 것을 막기 위한 안전장치.
	if (SummonedMinions.Num() >= MaxAliveMinions)
	{
		return;
	}

	const int32 SpawnCount = FMath::Min(SummonCountPerCast, MaxAliveMinions - SummonedMinions.Num());

	for (int32 i = 0; i < SpawnCount; ++i)
	{
		// 보스 주변 랜덤한 자리에 소환
		const float Angle = FMath::RandRange(0.f, 360.f);
		const float Distance = FMath::RandRange(SummonSpawnRadius * 0.5f, SummonSpawnRadius);
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Distance,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Distance,
			0.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AMonsterBase* NewMinion = GetWorld()->SpawnActor<AMonsterBase>(
			SummonClass, GetActorLocation() + Offset, GetActorRotation(), SpawnParams);

		if (NewMinion)
		{
			// ★ 보스가 타고 있는 스플라인을 그대로 물려줘서, 소환된 부하도
			// 코어를 향해 이동하게 함. 안 그러면 제자리에 멍하니 서 있음.
			if (SplineFollowComp && SplineFollowComp->TargetSpline && NewMinion->SplineFollowComp)
			{
				NewMinion->InitializeMonster(SplineFollowComp->TargetSpline, 1.f, 1.f);

				// 보스가 지금 있는 지점부터 출발 (처음부터 걸어오지 않도록)
				NewMinion->SplineFollowComp->SetDistance(SplineFollowComp->DistanceAlongSpline);
				NewMinion->SplineFollowComp->SetLateralOffsetSafe(FMath::RandRange(-200.f, 200.f));
			}

			SummonedMinions.Add(NewMinion);
		}
	}
}

void ABossMonsterBase::ThrowAxe()
{
	if (!AxeClass || CurrentState == EMonsterState::Dead || !GetWorld())
	{
		// ★ 진단용 로그 (원인 파악 후 지워도 됨)
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red,
			FString::Printf(TEXT("[도끼] 중단 - AxeClass: %s / 사망: %s"),
				AxeClass ? TEXT("있음") : TEXT("없음"),
				CurrentState == EMonsterState::Dead ? TEXT("예") : TEXT("아니오")));
		return;
	}

	// 레벨에서 기차를 찾음
	ATrainPawn* Train = nullptr;
	for (TActorIterator<ATrainPawn> It(GetWorld()); It; ++It)
	{
		Train = *It;
		if (Train) break;
	}

	if (!Train)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("[도끼] 중단 - 레벨에 기차가 없음"));
		return; // 기차가 없으면 던질 대상도 없음
	}

	// 너무 멀면 던지지 않음
	const FVector TrainLocation = Train->GetActorLocation();
	const float DistToTrain = FVector::Dist(GetActorLocation(), TrainLocation);
	if (DistToTrain > AxeThrowRange)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Orange,
			FString::Printf(TEXT("[도끼] 중단 - 기차가 너무 멂 (%.0f > %.0f)"), DistToTrain, AxeThrowRange));
		return;
	}

	const FVector SpawnLocation = GetActorLocation() + AxeSpawnOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	ABossAxeProjectile* Axe = GetWorld()->SpawnActor<ABossAxeProjectile>(
		AxeClass, SpawnLocation, GetActorRotation(), SpawnParams);

	if (Axe)
	{
		// ★ 던지는 시점의 기차 위치를 목표로 잡음. 기차가 계속 움직이므로
		// 플레이어가 이동해서 피할 수 있는 구조가 됨.
		Axe->LaunchAt(TrainLocation);
		OnAxeThrowStarted();
	}
}

void ABossMonsterBase::TriggerWarCry()
{
	if (CurrentState == EMonsterState::Dead || !GetWorld())
	{
		return;
	}

	// ★ 함성을 외치는 동안 제자리에 멈춤. 몽타주가 재생되는데 계속 걸어가면
	// 연출이 어색해지므로 이동과 공격을 잠시 중단시킴.
	bIsCasting = true;
	bIsCastingSkill = true; // Tick에서 덮어쓰지 않도록
	GetWorldTimerManager().ClearTimer(WarCryCastTimerHandle);
	GetWorldTimerManager().SetTimer(WarCryCastTimerHandle, this,
		&ABossMonsterBase::EndWarCryCast, WarCryCastTime, false);

	// 쇼크웨이브 이펙트
	if (WarCryEffect && WarCryEffectPoint)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), WarCryEffect, WarCryEffectPoint->GetComponentLocation());
	}

	// ★ 레벨의 모든 타워를 일정 시간 동안 공격 불가로 만듦.
	// 거리 제한 없이 전체에 걸리는 게 궁극기다운 위압감을 줌.
	for (TActorIterator<ATowerBase> It(GetWorld()); It; ++It)
	{
		if (ATowerBase* Tower = *It)
		{
			Tower->DisableForDuration(WarCryDuration);
		}
	}

	// ★ 병사 타워도 무력화 (배정 중단 + 연출). 소속 병사들은 아래에서 따로 처리됨.
	for (TActorIterator<ASoldierTower> It(GetWorld()); It; ++It)
	{
		if (ASoldierTower* SoldierTower = *It)
		{
			SoldierTower->DisableForDuration(WarCryDuration);
		}
	}

	// ★ 병사들도 같이 무력화. 타워만 멈추면 병사가 계속 싸워서
	// 궁극기의 위압감이 떨어짐.
	for (TActorIterator<ASoldierUnit> It(GetWorld()); It; ++It)
	{
		if (ASoldierUnit* Soldier = *It)
		{
			Soldier->DisableForDuration(WarCryDuration);
		}
	}

	// 플레이어도 같이 무력화. 기차에 타고 있으면 기차를, 아니면 캐릭터를 멈춤.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ATrainPawn* Train = Cast<ATrainPawn>(PC->GetPawn()))
		{
			Train->ApplyStun(WarCryDuration);
		}
		else if (ACH6_Shop_SimulatorCharacter* Character = Cast<ACH6_Shop_SimulatorCharacter>(PC->GetPawn()))
		{
			Character->ApplyStun(WarCryDuration);
		}
	}

	// 연출은 블루프린트에서 (몽타주, 사운드, 화면 효과 등)
	OnWarCryTriggered();
}

void ABossMonsterBase::EndWarCryCast()
{
	bIsCasting = false;
	bIsCastingSkill = false;
}

ADefenseObjectBase* ABossMonsterBase::FindRangedTarget() const
{
	ADefenseObjectBase* Nearest = nullptr;
	float NearestDistSq = FMath::Square(RangedAttackRange);

	for (TActorIterator<ADefenseObjectBase> It(GetWorld()); It; ++It)
	{
		ADefenseObjectBase* Target = *It;
		if (!IsValid(Target))
		{
			continue;
		}

		// ★ 파괴된 바리케이드는 Destroy가 아니라 Deactivate만 되어서 액터가
		// 남아있음. 비활성 상태면 대상에서 제외해야 부서진 벽을 계속
		// 때리는 일이 없음.
		if (const ADefenseWall* Wall = Cast<ADefenseWall>(Target))
		{
			if (!Wall->bIsActive)
			{
				continue;
			}
		}

		// 죽어가는 중인 병사도 제외
		if (const ASoldierUnit* Soldier = Cast<ASoldierUnit>(Target))
		{
			if (Soldier->IsDying())
			{
				continue;
			}
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Target;
		}
	}

	return Nearest;
}

void ABossMonsterBase::FireRangedAttack()
{
	if (!MagicProjectileClass || CurrentState == EMonsterState::Dead || !GetWorld())
	{
		return;
	}

	ADefenseObjectBase* Target = FindRangedTarget();
	if (!Target)
	{
		return; // 사거리 안에 칠 대상이 없음
	}

	const FVector TargetLocation = Target->GetActorLocation();

	// ★ 대상 쪽을 바라보게 회전. 스플라인 진행 방향만 보고 있으면 엉뚱한 데를
	// 향한 채로 쏘게 됨. 좌우(Yaw)만 돌려서 몸이 기울어지지 않게 함.
	// (시전 중에는 이동이 멈춰서 스플라인이 회전을 덮어쓰지 않으므로 유지됨)
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}

	// 회전을 먼저 맞춘 뒤에 위치를 계산해야 오프셋이 바라보는 방향 기준으로 적용됨
	const FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(MagicSpawnOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	ABossMagicProjectile* Magic = GetWorld()->SpawnActor<ABossMagicProjectile>(
		MagicProjectileClass, SpawnLocation, GetActorRotation(), SpawnParams);

	if (Magic)
	{
		Magic->LaunchTowards(TargetLocation, AttackDamage);

		// ★ 시전 모션 재생. 일반 공격 흐름을 타지 않으므로 여기서 직접 호출함.
		PlayAttackMontageOnAllMeshes();

		// 몽타주가 끝날 때까지 제자리에 멈춤 (쏘면서 걸어가면 어색함)
		StartSkillCast(1.f);

		OnRangedAttackFired();
	}
}

void ABossMonsterBase::StartSkillCast(float FallbackDuration)
{
	// ★ 몽타주 길이만큼 멈춤. 몽타주를 안 넣었으면 기본 시간을 씀.
	float CastDuration = FallbackDuration;
	if (AttackMontage)
	{
		CastDuration = AttackMontage->GetPlayLength();
	}

	bIsCastingSkill = true;
	bIsCasting = true;

	GetWorldTimerManager().ClearTimer(CastEndTimerHandle);
	GetWorldTimerManager().SetTimer(CastEndTimerHandle, this,
		&ABossMonsterBase::EndSkillCast, FMath::Max(0.1f, CastDuration), false);
}

void ABossMonsterBase::EndSkillCast()
{
	bIsCastingSkill = false;
	// bIsCasting은 Tick에서 원거리 조건에 따라 다시 판단됨
}

void ABossMonsterBase::UpdateAuraBuff()
{
	// ★ 주변 아군 몬스터에게 공격력/이동속도 버프.
	// 매번 곱하면 무한히 강해지므로, 이미 버프받은 몬스터는 건너뛰도록
	// 각 몬스터의 플래그로 관리함.
	for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
	{
		AMonsterBase* Ally = *It;
		if (!Ally || Ally == this || Ally->CurrentState == EMonsterState::Dead)
		{
			continue;
		}

		if (Ally->bHasBossBuff)
		{
			continue; // 이미 버프를 받은 몬스터
		}

		if (FVector::DistSquared(GetActorLocation(), Ally->GetActorLocation()) <= FMath::Square(BuffRadius))
		{
			Ally->AttackDamage *= BuffDamageMultiplier;

			// 이동속도는 SplineFollowComponent의 배율로 조절 (슬로우와 같은 방식)
			if (Ally->SplineFollowComp)
			{
				Ally->SplineFollowComp->SpeedMultiplier *= BuffSpeedMultiplier;
			}

			Ally->bHasBossBuff = true;
		}
	}
}