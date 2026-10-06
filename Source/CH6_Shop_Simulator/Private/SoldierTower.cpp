// Fill out your copyright notice in the Description page of Project Settings.

#include "SoldierTower.h"
#include "SoldierUnit.h"
#include "MonsterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "InteractableComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

ASoldierTower::ASoldierTower()
{
	// 범위 표시(bShowRangeDebug)를 위해서만 Tick이 필요함. 평소엔 아래 Tick
	// 안에서 바로 리턴하므로 부담이 거의 없음.
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	// 병사 3자리를 미리 만들어둠. 기본 위치는 대충 나란히 배치해두고,
	// 실제로는 블루프린트 에디터에서 길목에 맞춰 각각 옮겨서 쓰면 됨.
	for (int32 i = 0; i < 3; ++i)
	{
		USceneComponent* Slot = CreateDefaultSubobject<USceneComponent>(
			*FString::Printf(TEXT("SoldierSlot%d"), i));
		Slot->SetupAttachment(RootComponent);
		Slot->SetRelativeLocation(FVector(100.f, (i - 1) * 100.f, 0.f)); // 좌우로 나란히 기본 배치
		SoldierSlots.Add(Slot);
	}

	SpawnedSoldiers.SetNum(3);
	RespawnTimerHandles.SetNum(3);

	// ★ 감지 중심점. 기본값은 병사 슬롯 3개의 가운데쯤에 두고, 실제로는
	// 뷰포트에서 길목에 맞춰 옮겨서 쓰면 됨.
	DetectionCenter = CreateDefaultSubobject<USceneComponent>(TEXT("DetectionCenter"));
	DetectionCenter->SetupAttachment(RootComponent);
	DetectionCenter->SetRelativeLocation(FVector(100.f, 0.f, 0.f));

	// ★ 스턴 파티클이 나올 자리. 블루프린트에서 눈으로 보며 옮기면 됨.
	StunEffectPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StunEffectPoint"));
	StunEffectPoint->SetupAttachment(RootComponent);
	StunEffectPoint->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
}

void ASoldierTower::BeginPlay()
{
	Super::BeginPlay();

	// 시작하자마자 3자리 전부 즉시 채움 (초기 스폰은 딜레이 없이)
	for (int32 i = 0; i < SoldierSlots.Num(); ++i)
	{
		SpawnSoldierAtSlot(i);
	}

	GetWorldTimerManager().SetTimer(AssignTimerHandle, this, &ASoldierTower::AssignSoldiers, AssignInterval, true);

	// ★ 플레이어가 이 타워를 바라볼 때 감지 범위도 같이 보이도록 연동.
	// (InteractableComponent가 붙어있는 타워에만 적용됨 - 없으면 그냥 넘어감)
	if (UInteractableComponent* Interactable = FindComponentByClass<UInteractableComponent>())
	{
		Interactable->OnHighlightChanged.AddDynamic(this, &ASoldierTower::HandleHighlightChanged);
	}
}

void ASoldierTower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 범위 표시가 꺼져있으면 아무것도 안 함 (평소엔 여기서 바로 끝)
	if (!bShowRangeDebug)
	{
		return;
	}

	// ★ 실제 감지 판정과 같은 기준점(GroupCenter)에 그려야 함. 예전엔 타워
	// 위치에 그렸는데, 병사들이 타워 앞쪽에 배치되어 있으면 실제 판정 범위가
	// 그린 원보다 앞으로 밀려나 있어서 "원 밖인데 인식하네?" 하는 혼란이 있었음.
	DrawDebugCircle(GetWorld(), GetGroupCenter(), GroupDetectionRange, 48, FColor::Orange,
		false, -1.f, 0, 3.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
}

void ASoldierTower::DisableForDuration(float Duration)
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
		if (StunMontage)
		{
			// 몽타주가 짧으면 스턴이 끝나기 전에 재생이 먼저 끝나버리므로
			// 재생 속도를 조절해서 지속 시간에 맞춰 늘려줌
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

		if (StunEffect && StunEffectPoint)
		{
			ActiveStunEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
				StunEffect, StunEffectPoint, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset, false);
		}
	}

	GetWorldTimerManager().ClearTimer(DisableTimerHandle);
	GetWorldTimerManager().SetTimer(DisableTimerHandle, this, &ASoldierTower::EndDisable, Duration, false);
}

void ASoldierTower::EndDisable()
{
	bIsDisabled = false;

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

FVector ASoldierTower::GetGroupCenter() const
{
	// 고정 포인트를 기준으로 감지. 병사가 몇 명 살아있든 이 타워가 지키는
	// 구역이 항상 일정하게 유지됨.
	return DetectionCenter ? DetectionCenter->GetComponentLocation() : GetActorLocation();
}

void ASoldierTower::HandleHighlightChanged(bool bHighlighted)
{
	bShowRangeDebug = bHighlighted;
}

void ASoldierTower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ★ 타워가 판매/파괴될 때, 스폰해둔 병사들이 레벨에 그대로 남아있는 문제를 방지.
	// AActor::Destroy()는 스폰한 다른 액터들까지 자동으로 정리해주지 않으므로 직접 처리.
	for (ASoldierUnit* Soldier : SpawnedSoldiers)
	{
		if (IsValid(Soldier))
		{
			Soldier->Destroy();
		}
	}
	SpawnedSoldiers.Empty();

	// 대기 중이던 개별 부활 타이머들도 정리 (타워 자체가 사라졌으니)
	for (FTimerHandle& Handle : RespawnTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}

	Super::EndPlay(EndPlayReason);
}

void ASoldierTower::SpawnSoldierAtSlot(int32 SlotIndex)
{
	if (!SoldierClass || !SoldierSlots.IsValidIndex(SlotIndex) || !SoldierSlots[SlotIndex])
	{
		return;
	}

	// 이미 그 자리에 살아있는 병사가 있으면 건너뜀 (안전장치)
	if (SpawnedSoldiers.IsValidIndex(SlotIndex) && IsValid(SpawnedSoldiers[SlotIndex]))
	{
		return;
	}

	const FVector SpawnLoc = SoldierSlots[SlotIndex]->GetComponentLocation();

	// ★ 위치(SpawnLoc)는 슬롯 그대로 사용. 회전에만 FacingOffsetYaw를 더해서
	// "이 병사가 서있는 자리"는 안 바꾸고 "바라보는 방향"만 보정함.
	FRotator SpawnRot = SoldierSlots[SlotIndex]->GetComponentRotation();
	SpawnRot.Yaw += FacingOffsetYaw;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASoldierUnit* NewSoldier = GetWorld()->SpawnActor<ASoldierUnit>(SoldierClass, SpawnLoc, SpawnRot, SpawnParams);

	if (SpawnedSoldiers.IsValidIndex(SlotIndex))
	{
		SpawnedSoldiers[SlotIndex] = NewSoldier;
	}

	// ★ 이 병사가 나중에 죽으면(Destroy) HandleSoldierDestroyed가 호출되도록 등록.
	// 이게 있어야 "죽은 그 순간"부터 개별 타이머를 걸 수 있음 (주기적으로 살아있는지
	// 확인하는 방식이 아니라, 죽는 이벤트 자체를 직접 감지하는 방식).
	if (NewSoldier)
	{
		NewSoldier->OnDestroyed.AddDynamic(this, &ASoldierTower::HandleSoldierDestroyed);
	}
}

void ASoldierTower::HandleSoldierDestroyed(AActor* DestroyedActor)
{
	// 어느 슬롯 소속이었는지 찾음
	int32 SlotIndex = INDEX_NONE;
	for (int32 i = 0; i < SpawnedSoldiers.Num(); ++i)
	{
		if (SpawnedSoldiers[i] == DestroyedActor)
		{
			SlotIndex = i;
			break;
		}
	}

	if (SlotIndex == INDEX_NONE)
	{
		return; // 우리가 관리하는 슬롯 소속이 아니었음 (이론상 없어야 하지만 안전차원)
	}

	SpawnedSoldiers[SlotIndex] = nullptr;

	// ★ 죽은 이 순간부터 RespawnDelay만큼 카운트다운 시작. 이 슬롯 하나에 대해서만
	// 개별적으로 걸리는 타이머라, 다른 두 명은 전혀 영향 안 받음.
	if (RespawnTimerHandles.IsValidIndex(SlotIndex))
	{
		FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ASoldierTower::SpawnSoldierAtSlot, SlotIndex);
		GetWorldTimerManager().SetTimer(RespawnTimerHandles[SlotIndex], RespawnDelegate, RespawnDelay, false);
	}
}

void ASoldierTower::AssignSoldiers()
{
	// 무력화 중에는 새 배정을 하지 않음 (보스의 전쟁의 함성 등)
	if (bIsDisabled)
	{
		return;
	}

	// 지금 살아있는 병사만 모음
	// ★ IsDying()도 확인해야 함. 병사가 죽어도 사망 몽타주가 끝날 때까지
	// 액터는 남아있는데, 그동안 "살아있는 병사"로 잡히면 몬스터가 그 병사에게
	// 계속 배정되어 다른 병사가 대신 나서기까지 시간이 걸림.
	TArray<ASoldierUnit*> AliveSoldiers;
	for (ASoldierUnit* Soldier : SpawnedSoldiers)
	{
		if (IsValid(Soldier) && !Soldier->IsDying())
		{
			AliveSoldiers.Add(Soldier);
		}
	}
	if (AliveSoldiers.Num() == 0)
	{
		return;
	}

	// ★ 고정된 중심점(DetectionCenter) 기준으로 감지. 병사가 죽어도 감지 구역이
	// 변하지 않아서, "병사 한 명이 죽으면 그 구역이 사각지대가 되는" 문제가 없음.
	// (디버그 원도 같은 함수를 써서 실제 판정 범위와 항상 일치하게 함)
	const FVector GroupCenter = GetGroupCenter();

	TArray<AMonsterBase*> NearbyMonsters;
	for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
	{
		AMonsterBase* Monster = *It;
		if (!Monster || Monster->CurrentState == EMonsterState::Dead)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GroupCenter, Monster->GetActorLocation());
		if (DistSq <= FMath::Square(GroupDetectionRange))
		{
			NearbyMonsters.Add(Monster);
		}
	}

	if (NearbyMonsters.Num() == 0)
	{
		for (ASoldierUnit* Soldier : AliveSoldiers)
		{
			Soldier->ClearAssignedTarget();
		}
		return;
	}

	// 가까운 몬스터부터 우선 배정되도록 정렬 (같은 GroupCenter 기준)
	NearbyMonsters.Sort([GroupCenter](const AMonsterBase& A, const AMonsterBase& B)
		{
			return FVector::DistSquared(GroupCenter, A.GetActorLocation())
				< FVector::DistSquared(GroupCenter, B.GetActorLocation());
		});

	// ★ 핵심: 이번엔 "이미 붙잡은 병사는 절대 안 건드림"이 아니라, 커버 안 된
	// 몬스터가 있으면 "이미 여러 명이 몰려있는(과밀) 곳"에서 1명을 빼와서라도
	// 즉시 새 몬스터에게 보냄. 1:1로 이미 정확히 나뉜 상태(다들 딱 1명씩)라면
	// 더 뺄 여유가 없으니 그 몬스터는 커버 못 함 (병사 수 부족).
	TMap<AMonsterBase*, TArray<ASoldierUnit*>> AssignmentMap;
	TArray<ASoldierUnit*> IdleSoldiers;

	for (ASoldierUnit* Soldier : AliveSoldiers)
	{
		AMonsterBase* ExistingTarget = Soldier->GetCurrentTarget();
		const bool bTargetStillValid = ExistingTarget && IsValid(ExistingTarget)
			&& ExistingTarget->CurrentState != EMonsterState::Dead
			&& NearbyMonsters.Contains(ExistingTarget);

		if (bTargetStillValid)
		{
			// ★ 다굴 상황의 허점: 몬스터는 "내 상대"를 딱 1명만 기억하는데, 그
			// 1명이 죽으면 몬스터 입장에선 혼자 착각해서 Moving으로 풀려나버림.
			// 이 병사는 여전히 유효한 상대로 여기고 있으니, 몬스터가 실제로
			// 얼려있는(Attacking) 상태가 아니면 다시 한번 확실하게 재고정시킴.
			if (ExistingTarget->CurrentState != EMonsterState::Attacking)
			{
				Soldier->SetAssignedTarget(ExistingTarget);
			}
			AssignmentMap.FindOrAdd(ExistingTarget).Add(Soldier);
		}
		else
		{
			IdleSoldiers.Add(Soldier);
		}
	}

	// 1순위: 아직 아무도 없는 몬스터부터 노는 병사로 커버
	for (AMonsterBase* Monster : NearbyMonsters)
	{
		if (IdleSoldiers.Num() == 0)
		{
			break;
		}
		if (!AssignmentMap.Contains(Monster))
		{
			ASoldierUnit* Soldier = IdleSoldiers[0];
			IdleSoldiers.RemoveAt(0);
			Soldier->SetAssignedTarget(Monster);
			AssignmentMap.FindOrAdd(Monster).Add(Soldier);
		}
	}

	// 2순위: 노는 병사를 다 썼는데도 아직 아무도 없는 몬스터가 남아있으면,
	// 지금 가장 많은 병사가 몰려있는(과밀) 몬스터에서 1명을 빼와서 즉시 보냄.
	for (AMonsterBase* Monster : NearbyMonsters)
	{
		if (AssignmentMap.Contains(Monster))
		{
			continue; // 이미 커버됨
		}

		AMonsterBase* MostCrowdedMonster = nullptr;
		int32 MaxCount = 1; // 정확히 1명(1:1)인 곳은 안 건드림 - 더 빼면 그쪽이 무방비가 됨

		for (auto& Pair : AssignmentMap)
		{
			if (Pair.Value.Num() > MaxCount)
			{
				MaxCount = Pair.Value.Num();
				MostCrowdedMonster = Pair.Key;
			}
		}

		if (!MostCrowdedMonster)
		{
			continue; // 뺄 여유 있는 곳이 없음 -> 이 몬스터는 병사 수 부족으로 못 막음
		}

		TArray<ASoldierUnit*>& CrowdList = AssignmentMap[MostCrowdedMonster];
		ASoldierUnit* PulledSoldier = CrowdList.Last();
		CrowdList.RemoveAt(CrowdList.Num() - 1);

		PulledSoldier->SetAssignedTarget(Monster);
		AssignmentMap.FindOrAdd(Monster).Add(PulledSoldier);
	}

	// 혹시 남는 노는 병사가 있으면(이론상 거의 없지만 안전차원) 가까운 곳부터 합류
	int32 MonsterIndex = 0;
	while (IdleSoldiers.Num() > 0)
	{
		ASoldierUnit* Soldier = IdleSoldiers[0];
		IdleSoldiers.RemoveAt(0);
		AMonsterBase* Monster = NearbyMonsters[MonsterIndex % NearbyMonsters.Num()];
		Soldier->SetAssignedTarget(Monster);
		MonsterIndex++;
	}
}