// Fill out your copyright notice in the Description page of Project Settings.
//
// 데이터 테이블(FWaveData 행들)에 정의된 순서대로 웨이브를 자동 진행시키는 매니저.
// 스폰 완료 + 몬스터 전멸을 확인하면 인터미션 후 다음 웨이브로 자동 전환.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveData.h"
#include "WaveManager.generated.h"

class AMonsterSpawner;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWaveStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntermissionStarted, float, Duration);

// 특정 날짜부터 적용될 웨이브 순서 세트. 예: 1일차부터 [Wave_01,02,03], 4일차부터 [Wave_04,05,06]...
USTRUCT(BlueprintType)
struct FWaveSetForDayRange
{
	GENERATED_BODY()

	// 이 세트가 적용되기 시작하는 날짜 (이 날짜 이상이면 이 세트를 씀)
	UPROPERTY(EditAnywhere, Category = "Wave Set")
	int32 FromDay = 1;

	UPROPERTY(EditAnywhere, Category = "Wave Set")
	TArray<FName> WaveOrder;
};

// ★ 스포너 하나와 그 스포너 전용 웨이브 설정을 묶은 것. 이걸 여러 개 등록하면
// 스포너마다 완전히 다른 몬스터/난이도를 쓸 수 있음.
// (예: 왼쪽 스포너는 오크 테이블, 오른쪽 스포너는 해골 테이블)
USTRUCT(BlueprintType)
struct FSpawnerWaveConfig
{
	GENERATED_BODY()

	// 이 설정을 적용할 스포너
	UPROPERTY(EditAnywhere, Category = "Spawner Config")
	TObjectPtr<AMonsterSpawner> Spawner;

	// 이 스포너가 사용할 웨이브 데이터 테이블 (스포너마다 다르게 지정 가능)
	UPROPERTY(EditAnywhere, Category = "Spawner Config")
	TObjectPtr<UDataTable> WaveDataTable;

	// 이 스포너의 날짜 구간별 웨이브 순서
	UPROPERTY(EditAnywhere, Category = "Spawner Config")
	TArray<FWaveSetForDayRange> WaveOrderSets;

	// ★ 이 스포너가 몇 일차부터 활성화되는지. 안개로 막아둔 스포너는 이 값을
	// 11이나 21로 설정해두면, 그 날짜가 되기 전까지는 아예 웨이브를 안 돌림.
	UPROPERTY(EditAnywhere, Category = "Spawner Config")
	int32 ActiveFromDay = 1;

	// 진행 상태 (런타임용, 에디터에서 건드릴 필요 없음)
	int32 CurrentWaveIndex = 0;
};

UCLASS()
class CH6_SHOP_SIMULATOR_API AWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AWaveManager();

	// ★ 스포너별 웨이브 설정. 여기에 스포너를 여러 개 등록하면, 각각 다른
	// 데이터 테이블과 웨이브 순서를 써서 완전히 다른 몬스터가 나오게 할 수 있음.
	// 예: [0] 왼쪽 스포너 + DT_OrcWaves, [1] 오른쪽 스포너 + DT_SkeletonWaves
	UPROPERTY(EditAnywhere, Category = "Wave Manager")
	TArray<FSpawnerWaveConfig> SpawnerConfigs;

	// 웨이브 클리어 후 다음 웨이브 시작까지 대기 시간 (인터미션 - 타워 업그레이드/재배치 타이밍)
	UPROPERTY(EditAnywhere, Category = "Wave Manager")
	float IntermissionDuration = 15.f;

	// ★ true면 이전 웨이브 몬스터가 남아있어도 시간이 되면 다음 웨이브가 시작됨.
	// (몬스터가 계속 밀려와서 훨씬 급박한 느낌 - 타워디펜스에서 흔한 방식)
	// false면 이전 웨이브를 전멸시켜야 다음 웨이브가 시작됨.
	// ※ 마지막 웨이브는 이 옵션과 무관하게 항상 전멸을 기다린 뒤 밤이 끝남.
	UPROPERTY(EditAnywhere, Category = "Wave Manager")
	bool bContinuousWaves = true;

	// 스폰이 다 끝났는데도 몬스터가 안 죽고 이 시간(초)을 넘기면, 다 못 잡았어도
	// 강제로 다음 웨이브를 진행함. (몬스터가 어딘가 끼거나 남아있는 경우의 안전장치)
	UPROPERTY(EditAnywhere, Category = "Wave Manager")
	float WaveTimeoutDuration = 20.f;

	// 날짜가 지날수록 커지는 전역 배율. 낮 파트가 끝나고 밤이 시작될 때 GameMode 등에서 세팅.
	UPROPERTY(EditAnywhere, Category = "Wave Manager")
	float GlobalStatMultiplier = 1.f;

	// 밤이 시작될 때 호출 (예: GameTimeManager에서). 지금이 며칠째인지 알려줘야
	// 그 날짜에 맞는 웨이브 세트(WaveOrderSets)를 고를 수 있음.
	UFUNCTION(BlueprintCallable, Category = "Wave Manager")
	void StartNight(int32 InCurrentDay);

	// 날짜가 바뀔 때 호출해서 배율 갱신
	UFUNCTION(BlueprintCallable, Category = "Wave Manager")
	void SetDayMultiplier(float NewMultiplier) { GlobalStatMultiplier = NewMultiplier; }

	// 모든 웨이브가 끝났을 때 (밤 종료 = 낮으로 전환). C++/블루프린트 양쪽에서 구독 가능.
	UPROPERTY(BlueprintAssignable, Category = "Wave Manager")
	FOnAllWavesComplete OnAllWavesComplete;

	// 인터미션이 시작될 때 (남은 시간 UI 표시 등에 사용 가능)
	UPROPERTY(BlueprintAssignable, Category = "Wave Manager")
	FOnIntermissionStarted OnIntermissionStarted;

	// ★ 실제로 다음 웨이브가 시작되는 순간 (웨이브 번호 UI 갱신용).
	// 인터미션 시작 시점이 아니라 진짜 몬스터가 나오기 시작할 때 발생함.
	UPROPERTY(BlueprintAssignable, Category = "Wave Manager")
	FOnWaveStarted OnWaveStarted;

	// ★ UI 표시용: 지금 몇 번째 웨이브인지 (1부터 시작).
	// 스포너가 여러 개면 각자 진행도가 다를 수 있어서, 그중 가장 앞선 것을 기준으로 함.
	UFUNCTION(BlueprintPure, Category = "Wave Manager")
	int32 GetCurrentWaveNumber() const;

	// ★ UI 표시용: 오늘 밤 총 웨이브 수. 스포너별로 다르면 가장 많은 쪽을 기준으로 함.
	UFUNCTION(BlueprintPure, Category = "Wave Manager")
	int32 GetTotalWaveCount() const;

private:
	int32 CurrentDayForWaves = 1;
	float TimeSinceSpawnFinished = 0.f;

	// 특정 스포너의 특정 웨이브를 시작
	void StartWaveForSpawner(FSpawnerWaveConfig& Config, int32 WaveIndex);
	void CheckWaveCleared();
	void StartNextWave();

	// 해당 스포너 설정에서, 지금 날짜에 맞는 웨이브 순서 배열을 찾음
	const TArray<FName>& GetActiveWaveOrder(const FSpawnerWaveConfig& Config) const;

	// ★ 보스전 처리: 이번 밤에 보스가 등장했는지, 그리고 지금 살아있는지 확인.
	// 보스가 죽으면 남은 몬스터를 전부 정리하고 밤을 끝냄.
	bool bBossAppearedThisNight = false;
	bool CheckBossDefeated();
	void ClearAllRemainingMonsters();

	// 지금 날짜에 활성화된 스포너인지 (안개로 막혀있는 스포너 제외용)
	bool IsSpawnerActive(const FSpawnerWaveConfig& Config) const;

	FTimerHandle WaveCheckTimerHandle;
	FTimerHandle IntermissionTimerHandle;
};