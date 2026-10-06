// Fill out your copyright notice in the Description page of Project Settings.

#include "MonsterSpawner.h"
#include "BossMonsterBase.h"
#include "Components/SplineComponent.h"

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 지정된 경로 액터들에서 스플라인 컴포넌트를 미리 찾아서 캐싱
	CachedSplines.Reset();
	for (AActor* PathActor : SplinePathActors)
	{
		if (!PathActor)
		{
			continue;
		}

		if (USplineComponent* FoundSpline = PathActor->FindComponentByClass<USplineComponent>())
		{
			CachedSplines.Add(FoundSpline);
		}
	}

	if (CachedSplines.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("⚠ MonsterSpawner: 유효한 경로(스플라인)가 하나도 없습니다! Spline Path Actors 확인 필요"));
		return;
	}

	if (bAutoStartTestWaveOnBeginPlay)
	{
		StartWave(TestWaveData);
	}
}

USplineComponent* AMonsterSpawner::PickSpline(int32 PathIndex) const
{
	if (CachedSplines.Num() == 0)
	{
		return nullptr;
	}

	// ★ 웨이브 데이터에서 명시적으로 경로를 지정했으면 그걸 그대로 사용.
	// (같은 웨이브 안에서도 항목마다 다른 길로 보낼 수 있어서, "이 무리는
	// 왼쪽으로, 저 무리는 오른쪽으로" 같은 연출이 가능함)
	if (CachedSplines.IsValidIndex(PathIndex))
	{
		return CachedSplines[PathIndex];
	}

	// 지정 안 했거나(-1) 잘못된 인덱스면 랜덤으로 배정
	if (PathIndex >= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
			FString::Printf(TEXT("⚠ MonsterSpawner: PathIndex %d가 범위를 벗어나 랜덤 경로로 대체합니다. (등록된 경로 %d개)"),
				PathIndex, CachedSplines.Num()));
	}

	const int32 RandomIndex = FMath::RandRange(0, CachedSplines.Num() - 1);
	return CachedSplines[RandomIndex];
}

bool AMonsterSpawner::StartWave(const FWaveData& InWaveData)
{
	bHasStartedAnyWave = true;
	PendingSpawns.Reset();
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

	// 이미 죽어서 무효가 된 기록들을 정리 (목록이 무한정 쌓이는 것 방지)
	SpawnedMonsters.RemoveAll([](const TObjectPtr<AMonsterBase>& Monster)
		{
			return !IsValid(Monster);
		});

	if (CachedSplines.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("⚠ MonsterSpawner: StartWave 실패! 유효한 경로(스플라인)가 없습니다."));
		return false;
	}

	// ★ 여러 종류의 스폰 항목을 하나의 대기열로 순서대로 풀어냄.
	// 예: [일반x10, 정예x5] -> 대기열에 일반*10개 + 정예*5개가 순서대로 쌓임.
	for (const FMonsterSpawnEntry& Entry : InWaveData.SpawnEntries)
	{
		if (!Entry.MonsterClass)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange,
				TEXT("⚠ MonsterSpawner: 스폰 항목 중 Monster Class가 비어있는 게 있어 건너뜁니다."));
			continue;
		}

		for (int32 i = 0; i < Entry.Count; ++i)
		{
			FPendingSpawnItem Item;
			Item.MonsterClass = Entry.MonsterClass;
			Item.HealthMultiplier = Entry.HealthMultiplier;
			Item.DamageMultiplier = Entry.DamageMultiplier;
			Item.PathIndex = Entry.PathIndex; // 웨이브 데이터에서 지정한 경로를 그대로 전달
			PendingSpawns.Add(Item);
		}
	}

	if (PendingSpawns.Num() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			TEXT("⚠ MonsterSpawner: StartWave 실패! 유효한 스폰 항목이 하나도 없습니다. (데이터 테이블 확인)"));
		return false;
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMonsterSpawner::SpawnLoop,
		InWaveData.SpawnInterval, true /*반복*/, 0.f /*즉시 1번째 스폰*/);
	return true;
}

void AMonsterSpawner::SpawnLoop()
{
	if (PendingSpawns.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	// 대기열 맨 앞 항목을 꺼내서 스폰
	FPendingSpawnItem Item = PendingSpawns[0];
	PendingSpawns.RemoveAt(0);
	SpawnOneMonster(Item);
}

void AMonsterSpawner::SpawnOneMonster(const FPendingSpawnItem& Item)
{
	// ★ 웨이브 데이터에서 지정한 경로를 사용 (-1이면 랜덤)
	USplineComponent* ChosenSpline = PickSpline(Item.PathIndex);
	if (!Item.MonsterClass || !ChosenSpline)
	{
		return;
	}

	const FVector SpawnLoc = ChosenSpline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
	const FRotator SpawnRot = ChosenSpline->GetRotationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMonsterBase* NewMonster = GetWorld()->SpawnActor<AMonsterBase>(Item.MonsterClass, SpawnLoc, SpawnRot, SpawnParams);
	if (NewMonster)
	{
		NewMonster->InitializeMonster(ChosenSpline, Item.HealthMultiplier, Item.DamageMultiplier);

		// ★ 웨이브 클리어 판정에 쓰기 위해 자기가 스폰한 몬스터를 기록해둠
		SpawnedMonsters.Add(NewMonster);

		if (NewMonster->SplineFollowComp)
		{
			// ★ 보스는 길 한가운데로 당당하게 걸어오게 함. 일반 몬스터처럼
			// 좌우로 흩어지면 보스답지 않고, 등장 연출도 약해지기 때문.
			const bool bIsBoss = NewMonster->IsA(ABossMonsterBase::StaticClass());
			const float Offset = bIsBoss ? 0.f : FMath::RandRange(-400.f, 400.f);

			NewMonster->SplineFollowComp->SetLateralOffsetSafe(Offset);
		}
	}
}

void AMonsterSpawner::CancelPendingSpawns()
{
	PendingSpawns.Reset();
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}

bool AMonsterSpawner::HasAliveMonsters() const
{
	for (const TObjectPtr<AMonsterBase>& Monster : SpawnedMonsters)
	{
		if (IsValid(Monster) && Monster->CurrentState != EMonsterState::Dead)
		{
			return true;
		}
	}
	return false;
}