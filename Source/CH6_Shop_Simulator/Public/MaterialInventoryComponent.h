// Fill out your copyright notice in the Description page of Project Settings.
//
// 플레이어가 가진 재료를 관리하는 컴포넌트.
//
// 아이템 액터를 만들지 않고 "종류별 개수"만 들고 있는 단순한 구조.
// 재료를 바닥에 떨어뜨리고 줍는 과정이 없으므로 인벤토리 UI도 필요 없음
// (몬스터가 죽으면 바로 플레이어 보유량이 올라감).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingTypes.h"
#include "MaterialInventoryComponent.generated.h"

// 재료 보유량이 바뀔 때 UI에 알려주는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaterialChanged, EMaterialType, ChangedType, int32, NewAmount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CH6_SHOP_SIMULATOR_API UMaterialInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMaterialInventoryComponent();

	// ★ 재료 보유량이 바뀔 때 발생. 제작 UI나 HUD가 구독해서 갱신하면 됨.
	UPROPERTY(BlueprintAssignable, Category = "Material")
	FOnMaterialChanged OnMaterialChanged;

	// 게임 시작 시 지급할 재료 (튜토리얼용으로 철 몇 개 주는 식)
	UPROPERTY(EditDefaultsOnly, Category = "Material")
	TArray<FMaterialCost> StartingMaterials;

	// 재료를 추가 (몬스터 드랍, 상인 구매 등)
	UFUNCTION(BlueprintCallable, Category = "Material")
	void AddMaterial(EMaterialType Type, int32 Amount);

	// 재료를 소모. 부족하면 아무것도 안 하고 false를 반환함.
	UFUNCTION(BlueprintCallable, Category = "Material")
	bool ConsumeMaterials(const TArray<FMaterialCost>& Costs);

	// 지금 이 재료들을 낼 수 있는지 (제작 버튼 활성화 판단용)
	UFUNCTION(BlueprintPure, Category = "Material")
	bool CanAfford(const TArray<FMaterialCost>& Costs) const;

	// 특정 재료의 보유 개수
	UFUNCTION(BlueprintPure, Category = "Material")
	int32 GetAmount(EMaterialType Type) const;

	// ★ 이 재료들로 최대 몇 개까지 만들 수 있는지.
	// 제작 UI의 수량 조절 상한을 정할 때 씀.
	UFUNCTION(BlueprintPure, Category = "Material")
	int32 GetMaxCraftableCount(const TArray<FMaterialCost>& Costs) const;

	// 저장/불러오기용. 보유 중인 재료 전체를 배열로 주고받음.
	UFUNCTION(BlueprintCallable, Category = "Material")
	TArray<FMaterialCost> GetAllMaterials() const;

	UFUNCTION(BlueprintCallable, Category = "Material")
	void SetAllMaterials(const TArray<FMaterialCost>& InMaterials);

protected:
	virtual void BeginPlay() override;

private:
	// 종류별 보유 개수
	UPROPERTY()
	TMap<EMaterialType, int32> Materials;
};