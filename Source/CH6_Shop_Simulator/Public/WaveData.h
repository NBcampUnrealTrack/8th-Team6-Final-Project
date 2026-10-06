// Fill out your copyright notice in the Description page of Project Settings.
//
// 웨이브 하나에 여러 몬스터 종류를 섞어서 넣을 수 있도록 확장한 버전.
// 예: 일반 해골 x10 + 정예 해골 x5 를 한 웨이브에 같이 등록 가능.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WaveData.generated.h"

class AMonsterBase;

// 웨이브 안의 "스폰 항목" 하나 (몬스터 종류 하나 + 마릿수 + 배율)
USTRUCT(BlueprintType)
struct FMonsterSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Entry")
	TSubclassOf<AMonsterBase> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Entry")
	int32 Count = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Entry")
	float HealthMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Entry")
	float DamageMultiplier = 1.f;

	// ★ 이 몬스터들이 사용할 경로. 스포너의 Spline Path Actors 배열에서 몇 번째
	// 경로를 쓸지 지정함 (0 = 첫 번째 길, 1 = 두 번째 길...).
	// -1로 두면 매번 랜덤으로 배정 (기본값).
	// 예: 같은 웨이브에 [0]번 항목은 PathIndex=0(왼쪽 길), [1]번 항목은
	// PathIndex=1(오른쪽 길)로 넣으면 두 갈래로 나뉘어서 협공하는 연출 가능.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Entry")
	int32 PathIndex = -1;
};

USTRUCT(BlueprintType)
struct FWaveData : public FTableRowBase
{
	GENERATED_BODY()

	// 이 웨이브에 등장할 몬스터 종류들. 배열 순서대로 다 소진하며 스폰됨.
	// 예: [0]=일반 해골 x10, [1]=정예 해골 x5 -> 일반 10마리 다 나온 다음 정예 5마리가 이어서 나옴.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<FMonsterSpawnEntry> SpawnEntries;

	// 몇 초 간격으로 한 마리씩 스폰할지 (웨이브 전체에 공통 적용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	float SpawnInterval = 2.f;
};