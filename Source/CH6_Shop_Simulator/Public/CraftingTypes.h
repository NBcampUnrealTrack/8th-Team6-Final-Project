// Fill out your copyright notice in the Description page of Project Settings.
//
// 제작에 쓰이는 재료와 레시피 정의.
//
// 재료는 EMaterialType으로 구분하고, 플레이어는 종류별 보유 개수만 들고 있음
// (아이템 액터를 만들지 않으므로 인벤토리 UI 없이도 동작).
//
// 데이터 테이블 두 개를 만들어 쓰는 구조:
// - FMonsterDropData: 몬스터가 죽을 때 어떤 재료가 몇 퍼센트로 나오는지
// - FCraftRecipeData: 어떤 무기를 만들 때 재료가 얼마나 드는지

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CraftingTypes.generated.h"

// 재료 종류. 새 재료를 추가할 때 여기에 한 줄만 넣으면 됨.
UENUM(BlueprintType)
enum class EMaterialType : uint8
{
	Iron      UMETA(DisplayName = "철"),
	Gold      UMETA(DisplayName = "금"),
	BossCore  UMETA(DisplayName = "보스 결정")
};

// 레시피에서 "재료 하나가 몇 개 필요한지"를 나타내는 한 줄
USTRUCT(BlueprintType)
struct FMaterialCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMaterialType MaterialType = EMaterialType::Iron;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount = 1;
};

// ★ 몬스터 드랍 테이블의 한 행.
// 행 이름을 몬스터 블루프린트 이름과 맞춰두면 찾기 편함.
USTRUCT(BlueprintType)
struct FMonsterDropData : public FTableRowBase
{
	GENERATED_BODY()

	// 드랍할 재료 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMaterialType MaterialType = EMaterialType::Iron;

	// 드랍 확률 (0~1). 1이면 항상 드랍.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.f;

	// 드랍될 때 몇 개를 줄지 (최소~최대 사이 랜덤)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxAmount = 1;
};

// ★ 제작 레시피 테이블의 한 행.
USTRUCT(BlueprintType)
struct FCraftRecipeData : public FTableRowBase
{
	GENERATED_BODY()

	// 제작 목록에 표시될 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	// 목록에 표시될 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon;

	// 실제로 만들어질 상품 클래스 (AProductBase를 상속한 BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AProductBase> ProductClass;

	// 1개 만드는 데 드는 재료들
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMaterialCost> Costs;

	// 상세 설명 (판매가, 스탯 등을 적어두는 용도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText Description;

	// 한 번에 만들 수 있는 최대 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCraftCount = 99;
};