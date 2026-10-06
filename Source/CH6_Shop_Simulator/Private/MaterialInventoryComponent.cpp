// Fill out your copyright notice in the Description page of Project Settings.

#include "MaterialInventoryComponent.h"

UMaterialInventoryComponent::UMaterialInventoryComponent()
{
	// 매 프레임 할 일이 없음. 재료가 바뀔 때만 동작.
	PrimaryComponentTick.bCanEverTick = false;
}

void UMaterialInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 시작 재료 지급 (튜토리얼용)
	for (const FMaterialCost& Starting : StartingMaterials)
	{
		AddMaterial(Starting.MaterialType, Starting.Amount);
	}
}

void UMaterialInventoryComponent::AddMaterial(EMaterialType Type, int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	int32& Current = Materials.FindOrAdd(Type);
	Current += Amount;

	OnMaterialChanged.Broadcast(Type, Current);
}

int32 UMaterialInventoryComponent::GetAmount(EMaterialType Type) const
{
	const int32* Found = Materials.Find(Type);
	return Found ? *Found : 0;
}

bool UMaterialInventoryComponent::CanAfford(const TArray<FMaterialCost>& Costs) const
{
	for (const FMaterialCost& Cost : Costs)
	{
		if (GetAmount(Cost.MaterialType) < Cost.Amount)
		{
			return false;
		}
	}
	return true;
}

bool UMaterialInventoryComponent::ConsumeMaterials(const TArray<FMaterialCost>& Costs)
{
	// ★ 먼저 전부 낼 수 있는지 확인한 뒤에 소모함.
	// 하나씩 차감하다가 중간에 부족하면 이미 깎인 재료가 날아가버림.
	if (!CanAfford(Costs))
	{
		return false;
	}

	for (const FMaterialCost& Cost : Costs)
	{
		int32& Current = Materials.FindOrAdd(Cost.MaterialType);
		Current -= Cost.Amount;

		OnMaterialChanged.Broadcast(Cost.MaterialType, Current);
	}

	return true;
}

int32 UMaterialInventoryComponent::GetMaxCraftableCount(const TArray<FMaterialCost>& Costs) const
{
	if (Costs.Num() == 0)
	{
		return 0;
	}

	// ★ 가장 빠듯한 재료가 제작 가능 개수를 결정함.
	// (철 10개 / 금 2개를 갖고 있고 레시피가 철3+금1이면 금 때문에 2개까지)
	int32 MaxCount = MAX_int32;

	for (const FMaterialCost& Cost : Costs)
	{
		if (Cost.Amount <= 0)
		{
			continue; // 0개 필요한 항목은 제한이 되지 않음
		}

		const int32 Possible = GetAmount(Cost.MaterialType) / Cost.Amount;
		MaxCount = FMath::Min(MaxCount, Possible);
	}

	return (MaxCount == MAX_int32) ? 0 : MaxCount;
}

TArray<FMaterialCost> UMaterialInventoryComponent::GetAllMaterials() const
{
	TArray<FMaterialCost> Result;

	for (const TPair<EMaterialType, int32>& Pair : Materials)
	{
		FMaterialCost Entry;
		Entry.MaterialType = Pair.Key;
		Entry.Amount = Pair.Value;
		Result.Add(Entry);
	}

	return Result;
}

void UMaterialInventoryComponent::SetAllMaterials(const TArray<FMaterialCost>& InMaterials)
{
	// ★ 불러온 값으로 통째로 교체. BeginPlay에서 지급된 시작 재료가
	// 있더라도 여기서 덮어쓰므로 중복 지급되지 않음.
	Materials.Empty();

	for (const FMaterialCost& Entry : InMaterials)
	{
		Materials.Add(Entry.MaterialType, Entry.Amount);
		OnMaterialChanged.Broadcast(Entry.MaterialType, Entry.Amount);
	}
}