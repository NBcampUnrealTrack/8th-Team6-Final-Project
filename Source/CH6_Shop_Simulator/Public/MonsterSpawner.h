// Fill out your copyright notice in the Description page of Project Settings.
//
// 이제 "고정된 몬스터 1종류 반복 스폰"이 아니라, 외부(WaveManager 등)에서
// StartWave()로 FWaveData를 넘겨주면 그 내용대로 스폰하는 구조로 확장.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterBase.h"
#include "WaveData.h"
#include "MonsterSpawner.generated.h"

class USplineComponent;

// 실제로 하나씩 꺼내 쓸 스폰 대기열 항목. 블루프린트에 노출할 필요 없어서 USTRUCT 없이 사용.
struct FPendingSpawnItem
{
	TSubclassOf<AMonsterBase> MonsterClass;
	float HealthMultiplier = 1.f;
	float DamageMultiplier = 1.f;
	int32 PathIndex = -1; // -1이면 랜덤, 아니면 SplinePathActors 배열의 해당 인덱스 사용
};

UCLASS()
class CH6_SHOP_SIMULATOR_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

protected:
	virtual void BeginPlay() override;

	// ★ 이 스포너에서 나온 몬스터들이 사용할 경로들. 여러 개 지정하면 몬스터마다
	// 랜덤으로 하나씩 골라서 배정되므로, 같은 스포너에서 나와도 여러 갈래로
	// 흩어져서 이동함 (맵에 갈림길이 여러 개 있는 구조에 대응).
	// 스플라인 컴포넌트를 가진 레벨 액터들을 에디터에서 드래그해서 넣으면 됨.
	UPROPERTY(EditAnywhere, Category = "Spawner Config")
	TArray<AActor*> SplinePathActors;

	// 테스트용: 게임 시작하자마자 바로 웨이브 하나를 자동으로 돌려볼 수 있게.
	// 나중에 WaveManager가 생기면 이 웨이브 지정 대신 WaveManager가 StartWave()를 호출하게 됨.
	UPROPERTY(EditAnywhere, Category = "Spawner Config | Test")
	bool bAutoStartTestWaveOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category = "Spawner Config | Test", meta = (EditCondition = "bAutoStartTestWaveOnBeginPlay"))
	FWaveData TestWaveData;

public:
	// 외부(WaveManager)에서 호출: 이 웨이브 데이터대로 스폰을 시작.
	// 실패하면 false를 반환 (스플라인/데이터 누락 등 설정 문제를 호출부가 알 수 있게).
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool StartWave(const FWaveData& InWaveData);

	// 이번에 요청받은 웨이브의 스폰이 전부 끝났는지 (대기열이 비었는지)
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	bool HasFinishedSpawning() const { return bHasStartedAnyWave && PendingSpawns.Num() == 0; }

	// ★ 이 스포너가 스폰한 몬스터 중 아직 살아있는 게 있는지.
	// 레벨 전체를 훑는 대신 자기가 만든 목록만 확인하므로, 테스트용으로 레벨에
	// 미리 배치해둔 몬스터 때문에 웨이브 판정이 꼬이는 일이 없고 더 가벼움.
	bool HasAliveMonsters() const;

	// ★ 아직 스폰되지 않은 대기열을 전부 비움. 보스를 잡아서 밤이 조기
	// 종료될 때, 남은 졸개들이 계속 나오지 않게 하기 위함.
	void CancelPendingSpawns();

private:
	void SpawnLoop();
	void SpawnOneMonster(const FPendingSpawnItem& Item);

	FTimerHandle SpawnTimerHandle;

	UPROPERTY()
	TArray<TObjectPtr<USplineComponent>> CachedSplines;

	// 지정된 인덱스의 경로를 반환. -1이면 랜덤으로 하나 고름.
	// 인덱스가 범위를 벗어나면 안전하게 랜덤으로 대체됨.
	USplineComponent* PickSpline(int32 PathIndex) const;

	TArray<FPendingSpawnItem> PendingSpawns;

	// 이 스포너가 스폰한 몬스터들. 죽거나 파괴되면 자동으로 무효(nullptr)가 되므로
	// IsValid로 걸러서 살아있는 수를 셈.
	UPROPERTY()
	TArray<TObjectPtr<AMonsterBase>> SpawnedMonsters;
	bool bHasStartedAnyWave = false; // StartWave가 한 번이라도 호출되기 전엔 "끝남" 취급 안 하기 위한 플래그
};