// Fill out your copyright notice in the Description page of Project Settings.
//
// 하루가 시작될 때마다 자동 저장되는 데이터.
// 메인 메뉴의 LOAD GAME에서 이 데이터를 읽어 이어하기를 함.
//
// 저장 범위는 "다시 시작했을 때 진행 상황이 유지되는" 최소한으로 잡음:
// 날짜, 골드, 지어둔 타워들. 진열대나 상품 같은 낮 파트 상태는
// 구조가 정리된 뒤에 추가하면 됨.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CraftingTypes.h"
#include "ShopSaveGame.generated.h"

// 지어둔 구조물 하나의 정보.
// 어느 타일에 무엇을 지었는지만 알면 로드할 때 그대로 복원할 수 있음.
USTRUCT()
struct FSavedStructure
{
	GENERATED_BODY()

	// 타일을 구분하는 이름. 레벨에 배치된 액터의 이름을 그대로 씀.
	UPROPERTY()
	FName TileName;

	// 그 타일에 지어진 구조물 클래스 경로
	UPROPERTY()
	FSoftClassPath StructureClassPath;

	// 지을 때 낸 비용 (판매 환불 계산에 필요)
	UPROPERTY()
	int32 PaidCost = 0;
};

UCLASS()
class CH6_SHOP_SIMULATOR_API UShopSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 저장된 시점의 날짜 (이 날 아침부터 다시 시작)
	UPROPERTY()
	int32 SavedDay = 1;

	// 보유 골드
	UPROPERTY()
	int32 SavedGold = 0;

	// ★ 플레이어 스탯. 최종값(FinalAttackPower 등)은 이 값들로부터 계산되므로
	// 원본만 저장하면 됨. 이게 없으면 이어하기 때 낮에 올려둔 스탯이
	// 전부 초기화되어버림.
	UPROPERTY()
	int32 SavedBaseAttackPower = 10;

	UPROPERTY()
	float SavedAttackMultiplier = 1.f;

	UPROPERTY()
	int32 SavedBaseMaxHealth = 100;

	UPROPERTY()
	float SavedHealthMultiplier = 1.f;

	// ★ 보유 재료. 몬스터를 잡아 모은 것들이라 이게 없으면
	// 이어하기 때 제작을 처음부터 다시 해야 함.
	// (TMap 대신 배열로 저장 - 세이브 파일 직렬화가 더 안정적임)
	UPROPERTY()
	TArray<FMaterialCost> SavedMaterials;

	// 지어둔 구조물 목록
	UPROPERTY()
	TArray<FSavedStructure> SavedStructures;

	// 저장 슬롯 이름 (여러 곳에서 같은 값을 쓰도록 여기 모아둠)
	static const FString SlotName;
};