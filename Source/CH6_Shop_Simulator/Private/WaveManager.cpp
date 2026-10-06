// Fill out your copyright notice in the Description page of Project Settings.

#include "WaveManager.h"
#include "MonsterSpawner.h"
#include "MonsterBase.h"
#include "BossMonsterBase.h"
#include "Engine/DataTable.h"
#include "EngineUtils.h"

AWaveManager::AWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWaveManager::StartNight(int32 InCurrentDay)
{
	CurrentDayForWaves = InCurrentDay;
	bBossAppearedThisNight = false;

	// ★ 활성화된 모든 스포너의 웨이브를 동시에 시작. 스포너마다 자기 진행도를
	// 따로 들고 있어서, 서로 다른 속도로 웨이브가 흘러가도 문제없음.
	for (FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		Config.CurrentWaveIndex = 0;

		if (IsSpawnerActive(Config))
		{
			StartWaveForSpawner(Config, 0);
		}
	}

	// 첫 웨이브가 시작됐음을 알림 (UI가 1/N부터 표시되도록)
	OnWaveStarted.Broadcast();

	// 스폰 완료 + 몬스터 전멸 여부를 저빈도로 확인 (1초마다 - 부담 없는 수준)
	GetWorldTimerManager().SetTimer(WaveCheckTimerHandle, this, &AWaveManager::CheckWaveCleared, 1.f, true);
}

bool AWaveManager::IsSpawnerActive(const FSpawnerWaveConfig& Config) const
{
	// 안개로 막아둔 스포너는 ActiveFromDay를 11, 21 등으로 설정해두면
	// 그 날짜가 되기 전까지 아예 웨이브를 안 돌림
	return Config.Spawner && CurrentDayForWaves >= Config.ActiveFromDay;
}

const TArray<FName>& AWaveManager::GetActiveWaveOrder(const FSpawnerWaveConfig& Config) const
{
	static TArray<FName> EmptyArray;
	const FWaveSetForDayRange* BestMatch = nullptr;

	for (const FWaveSetForDayRange& Set : Config.WaveOrderSets)
	{
		if (Set.FromDay <= CurrentDayForWaves)
		{
			if (!BestMatch || Set.FromDay > BestMatch->FromDay)
			{
				BestMatch = &Set;
			}
		}
	}

	return BestMatch ? BestMatch->WaveOrder : EmptyArray;
}

void AWaveManager::StartWaveForSpawner(FSpawnerWaveConfig& Config, int32 WaveIndex)
{
	const TArray<FName>& ActiveWaveOrder = GetActiveWaveOrder(Config);

	if (!Config.WaveDataTable || !Config.Spawner || !ActiveWaveOrder.IsValidIndex(WaveIndex))
	{
		return; // 이 스포너는 더 이상 돌릴 웨이브가 없음 (다른 스포너는 계속 진행)
	}

	FWaveData* RowData = Config.WaveDataTable->FindRow<FWaveData>(
		ActiveWaveOrder[WaveIndex], TEXT("AWaveManager::StartWaveForSpawner"));

	if (!RowData)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			FString::Printf(TEXT("⚠ WaveManager: '%s' 행을 데이터 테이블에서 찾을 수 없습니다."),
				*ActiveWaveOrder[WaveIndex].ToString()));
		return;
	}

	// 원본 데이터는 안 건드리고 복사해서 날짜 배율만 곱함 (각 스폰 항목마다 개별로 곱해줘야 함)
	FWaveData ScaledWave = *RowData;
	for (FMonsterSpawnEntry& Entry : ScaledWave.SpawnEntries)
	{
		Entry.HealthMultiplier *= GlobalStatMultiplier;
		Entry.DamageMultiplier *= GlobalStatMultiplier;
	}

	if (!Config.Spawner->StartWave(ScaledWave))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
			FString::Printf(TEXT("⚠ WaveManager: '%s' 웨이브 시작 실패 (스포너 설정 확인)"),
				*ActiveWaveOrder[WaveIndex].ToString()));
	}
}

void AWaveManager::CheckWaveCleared()
{
	// ★ 보스전 처리가 최우선. 보스를 잡았으면 남은 웨이브나 졸개와 상관없이
	// 그 즉시 밤이 끝남.
	if (CheckBossDefeated())
	{
		ClearAllRemainingMonsters();

		TimeSinceSpawnFinished = 0.f;
		bBossAppearedThisNight = false;
		GetWorldTimerManager().ClearTimer(WaveCheckTimerHandle);
		GetWorldTimerManager().ClearTimer(IntermissionTimerHandle);

		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Yellow, TEXT("보스 격파! 밤이 끝납니다."));

		// ★ 밤이 끝났으니 웨이브 인덱스를 되돌림. 안 하면 다음 밤이 시작되기
		// 전까지 UI에 지난 밤의 마지막 번호(3/3)가 그대로 남아있음.
		for (FSpawnerWaveConfig& Config : SpawnerConfigs)
		{
			Config.CurrentWaveIndex = 0;
		}

		OnAllWavesComplete.Broadcast();
		return;
	}

	// ★ 활성화된 스포너들이 "전부" 스폰을 끝냈는지 확인. 하나라도 아직 스폰 중이면
	// 이번 밤은 아직 안 끝난 것.
	bool bAllFinishedSpawning = true;
	for (const FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config) && !Config.Spawner->HasFinishedSpawning())
		{
			bAllFinishedSpawning = false;
			break;
		}
	}

	if (!bAllFinishedSpawning)
	{
		TimeSinceSpawnFinished = 0.f; // 아직 스폰 중이면 타임아웃 카운트는 시작 안 함
		return;
	}

	// ★ 다음 웨이브가 남아있는지 먼저 확인. 연속 모드에서는 남은 웨이브가 있으면
	// 전멸을 기다리지 않고 바로 다음 웨이브로 넘어가서 몬스터가 계속 밀려오게 함.
	bool bHasNextWave = false;
	for (const FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config)
			&& GetActiveWaveOrder(Config).IsValidIndex(Config.CurrentWaveIndex + 1))
		{
			bHasNextWave = true;
			break;
		}
	}

	// 연속 모드 + 다음 웨이브가 남아있으면 전멸 확인 자체를 건너뜀.
	// (마지막 웨이브는 항상 전멸을 기다려야 밤이 제대로 끝남)
	const bool bNeedAllDead = !bContinuousWaves || !bHasNextWave;

	if (bNeedAllDead)
	{
		// ★ 활성 스포너들이 스폰한 몬스터 중 살아있는 게 있는지 확인.
		// 예전엔 레벨 전체를 훑었는데, 그러면 테스트용으로 미리 배치해둔 몬스터까지
		// 카운트되어 웨이브가 영영 안 끝나는 문제가 있었음. 스포너가 자기 목록만
		// 확인하므로 더 정확하고 가벼움.
		bool bAnyAlive = false;
		for (const FSpawnerWaveConfig& Config : SpawnerConfigs)
		{
			if (IsSpawnerActive(Config) && Config.Spawner->HasAliveMonsters())
			{
				bAnyAlive = true;
				break;
			}
		}

		if (bAnyAlive)
		{
			// 폴링 간격(1초)만큼 누적. 타임아웃 전이면 계속 대기.
			TimeSinceSpawnFinished += 1.f;
			if (TimeSinceSpawnFinished < WaveTimeoutDuration)
			{
				return;
			}

			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
				TEXT("⏱ 웨이브 타임아웃 - 남은 몬스터가 있어도 강제로 진행합니다."));
		}
	}

	// 이번 웨이브 종료 처리 (전멸했거나, 연속 모드라 바로 넘어가거나, 타임아웃)
	TimeSinceSpawnFinished = 0.f;
	GetWorldTimerManager().ClearTimer(WaveCheckTimerHandle);

	// ★ 여기서는 "다음 웨이브가 남아있는지"만 확인하고, 실제 인덱스 증가는
	// 인터미션이 끝난 뒤(StartNextWave)에 함. 여기서 미리 올려버리면 인터미션
	// 대기 중인데도 UI가 다음 웨이브 번호로 먼저 바뀌어버림.
	if (bHasNextWave)
	{
		OnIntermissionStarted.Broadcast(IntermissionDuration);
		GetWorldTimerManager().SetTimer(IntermissionTimerHandle, this, &AWaveManager::StartNextWave, IntermissionDuration, false);
	}
	else
	{
		// ★ 밤이 끝났으니 웨이브 인덱스를 되돌림. 안 하면 다음 밤이 시작되기
		// 전까지 UI에 지난 밤의 마지막 번호(3/3)가 그대로 남아있음.
		for (FSpawnerWaveConfig& Config : SpawnerConfigs)
		{
			Config.CurrentWaveIndex = 0;
		}

		OnAllWavesComplete.Broadcast();
	}
}

int32 AWaveManager::GetCurrentWaveNumber() const
{
	// ★ 스포너마다 웨이브 개수가 다를 수 있어서, 진행이 가장 앞선 쪽을 기준으로 표시.
	// (CurrentWaveIndex는 0부터 시작하므로 +1해서 사람이 읽는 번호로 바꿈)
	int32 MaxIndex = 0;
	for (const FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config))
		{
			MaxIndex = FMath::Max(MaxIndex, Config.CurrentWaveIndex);
		}
	}
	// ★ 마지막 웨이브를 클리어하면 인덱스가 총 개수를 넘어서는데(3개짜리에서 3이 됨),
	// 그대로 표시하면 "4/3"처럼 보이므로 총 개수 이하로 제한함.
	const int32 Total = GetTotalWaveCount();
	return (Total > 0) ? FMath::Min(MaxIndex + 1, Total) : (MaxIndex + 1);
}

int32 AWaveManager::GetTotalWaveCount() const
{
	// 활성 스포너들 중 웨이브가 가장 많은 쪽을 오늘 밤의 총 웨이브 수로 봄
	int32 MaxCount = 0;
	for (const FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config))
		{
			MaxCount = FMath::Max(MaxCount, GetActiveWaveOrder(Config).Num());
		}
	}
	return MaxCount;
}

bool AWaveManager::CheckBossDefeated()
{
	// ★ 레벨에 살아있는 보스가 있는지 확인. 한 번이라도 보스를 본 적이 있는데
	// 지금은 없다면, 보스를 잡은 것으로 판단함.
	bool bAnyBossAlive = false;
	for (TActorIterator<ABossMonsterBase> It(GetWorld()); It; ++It)
	{
		ABossMonsterBase* Boss = *It;
		if (Boss && Boss->bEndNightOnDeath && Boss->CurrentState != EMonsterState::Dead)
		{
			bAnyBossAlive = true;
			bBossAppearedThisNight = true; // 이번 밤에 보스가 등장했음을 기록
			break;
		}
	}

	// 등장했었는데 지금 없으면 = 잡힘
	return bBossAppearedThisNight && !bAnyBossAlive;
}

void AWaveManager::ClearAllRemainingMonsters()
{
	// 보스를 잡았으니 남은 졸개들은 전부 정리
	for (TActorIterator<AMonsterBase> It(GetWorld()); It; ++It)
	{
		AMonsterBase* Monster = *It;
		if (Monster && Monster->CurrentState != EMonsterState::Dead)
		{
			Monster->Destroy();
		}
	}

	// 스포너의 대기열에 아직 안 나온 몬스터들도 취소
	for (FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (Config.Spawner)
		{
			Config.Spawner->CancelPendingSpawns();
		}
	}
}

void AWaveManager::StartNextWave()
{
	// ★ 인터미션이 끝난 지금 시점에 인덱스를 올림. 그래야 UI의 웨이브 번호가
	// "대기 중"이 아니라 "실제로 다음 웨이브가 시작되는 순간"에 바뀜.
	for (FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config))
		{
			Config.CurrentWaveIndex++;
		}
	}

	// 각 스포너가 자기 다음 웨이브를 시작 (이미 끝난 스포너는 StartWaveForSpawner
	// 안에서 유효 인덱스 체크에 걸려 자동으로 건너뜀)
	for (FSpawnerWaveConfig& Config : SpawnerConfigs)
	{
		if (IsSpawnerActive(Config))
		{
			StartWaveForSpawner(Config, Config.CurrentWaveIndex);
		}
	}

	// 웨이브 번호가 바뀌었으니 UI 갱신 (GameTimeManager가 이 신호를 구독 중)
	OnWaveStarted.Broadcast();

	GetWorldTimerManager().SetTimer(WaveCheckTimerHandle, this, &AWaveManager::CheckWaveCleared, 1.f, true);
}