// Fill out your copyright notice in the Description page of Project Settings.

#include "CraftingWidget.h"
#include "CraftingAnvil.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "MaterialInventoryComponent.h"
#include "Engine/DataTable.h"

void UCraftingWidget::SetupForAnvil(ACraftingAnvil* InAnvil, ACH6_Shop_SimulatorCharacter* InUser)
{
	OwningAnvil = InAnvil;
	User = InUser;

	// ★ 재료가 바뀌면 자동으로 화면을 갱신하도록 구독.
	// 제작으로 재료가 줄거나, 메뉴를 연 채로 몬스터를 잡아 늘어날 때 반영됨.
	if (User && User->MaterialInventory)
	{
		User->MaterialInventory->OnMaterialChanged.AddDynamic(this, &UCraftingWidget::HandleMaterialChanged);
	}

	// ★ 제작대가 연결된 지금 시점에 목록을 만들어야 함.
	// Event Construct는 AddToViewport 때 실행되는데, 그때는 아직
	// OwningAnvil이 비어있어서 레시피 목록을 가져올 수 없음.
	OnAnvilReady();

	// 첫 항목을 기본 선택해서 빈 화면으로 시작하지 않게 함
	const TArray<FName> Recipes = GetAvailableRecipes();
	if (Recipes.Num() > 0)
	{
		SelectRecipe(Recipes[0]);
	}
}

TArray<FName> UCraftingWidget::GetAvailableRecipes() const
{
	return OwningAnvil ? OwningAnvil->GetRecipeRowNames() : TArray<FName>();
}

bool UCraftingWidget::GetRecipeData(FName RecipeRowName, FCraftRecipeData& OutData) const
{
	if (!OwningAnvil || !OwningAnvil->RecipeDataTable)
	{
		return false;
	}

	const FCraftRecipeData* Found =
		OwningAnvil->RecipeDataTable->FindRow<FCraftRecipeData>(RecipeRowName, TEXT("UI"));

	if (!Found)
	{
		return false;
	}

	OutData = *Found;
	return true;
}

void UCraftingWidget::SelectRecipe(FName RecipeRowName)
{
	SelectedRecipeName = RecipeRowName;
	CraftCount = 1; // 다른 걸 고르면 수량은 1부터 다시

	OnSelectionUpdated();
}

void UCraftingWidget::ChangeCraftCount(int32 Delta)
{
	const int32 MaxCount = GetMaxCraftCount();

	// ★ 1개 미만으로는 못 내려가고, 만들 수 있는 최대치를 넘지도 못함.
	// 재료가 아예 없으면 최대치가 0이라 1로 고정됨 (제작 버튼이 비활성화될 뿐).
	const int32 UpperLimit = FMath::Max(1, MaxCount);
	CraftCount = FMath::Clamp(CraftCount + Delta, 1, UpperLimit);

	OnSelectionUpdated();
}

TArray<FMaterialCost> UCraftingWidget::GetCurrentTotalCosts() const
{
	TArray<FMaterialCost> Result;

	FCraftRecipeData Recipe;
	if (!GetRecipeData(SelectedRecipeName, Recipe))
	{
		return Result;
	}

	// 1개당 비용에 수량을 곱한 값
	for (const FMaterialCost& Cost : Recipe.Costs)
	{
		FMaterialCost Scaled = Cost;
		Scaled.Amount = Cost.Amount * CraftCount;
		Result.Add(Scaled);
	}

	return Result;
}

int32 UCraftingWidget::GetOwnedAmount(EMaterialType Type) const
{
	if (!User || !User->MaterialInventory)
	{
		return 0;
	}
	return User->MaterialInventory->GetAmount(Type);
}

TArray<FMaterialCost> UCraftingWidget::GetOwnedMaterials() const
{
	TArray<FMaterialCost> Result;

	if (!User || !User->MaterialInventory)
	{
		return Result;
	}

	// 0개인 재료는 빼고 돌려줌 (목록에 "철 0" 같은 게 뜨지 않도록)
	for (const FMaterialCost& Entry : User->MaterialInventory->GetAllMaterials())
	{
		if (Entry.Amount > 0)
		{
			Result.Add(Entry);
		}
	}

	return Result;
}

bool UCraftingWidget::CanCraftNow() const
{
	if (!User || !User->MaterialInventory)
	{
		return false;
	}

	// 이미 작업 중이면 새 주문을 못 넣음
	if (IsAnvilBusy())
	{
		return false;
	}

	return User->MaterialInventory->CanAfford(GetCurrentTotalCosts());
}

bool UCraftingWidget::IsAnvilBusy() const
{
	return OwningAnvil ? OwningAnvil->IsCrafting() : false;
}

int32 UCraftingWidget::GetRemainingCraftCount() const
{
	return OwningAnvil ? OwningAnvil->GetRemainingCraftCount() : 0;
}

int32 UCraftingWidget::GetMaxCraftCount() const
{
	FCraftRecipeData Recipe;
	if (!GetRecipeData(SelectedRecipeName, Recipe) || !User || !User->MaterialInventory)
	{
		return 0;
	}

	// 재료로 만들 수 있는 수량과 레시피가 정한 상한 중 작은 쪽
	const int32 ByMaterial = User->MaterialInventory->GetMaxCraftableCount(Recipe.Costs);
	return FMath::Min(ByMaterial, Recipe.MaxCraftCount);
}

void UCraftingWidget::ConfirmCraft()
{
	if (!OwningAnvil || SelectedRecipeName.IsNone())
	{
		return;
	}

	if (OwningAnvil->CraftProduct(SelectedRecipeName, CraftCount))
	{
		// ★ 제작 성공 - 재료가 줄었으므로 수량을 가능한 범위로 다시 맞춤.
		// 재료 소모 시점에 델리게이트가 먼저 발생하는데, 그때는 아직 보정 전이라
		// 화면에 옛 수량이 남음. 그래서 보정 후 한 번 더 갱신해줌.
		const int32 MaxCount = GetMaxCraftCount();
		CraftCount = FMath::Clamp(CraftCount, 1, FMath::Max(1, MaxCount));

		OnSelectionUpdated();
	}
}

void UCraftingWidget::HandleMaterialChanged(EMaterialType ChangedType, int32 NewAmount)
{
	// 보유 재료 목록과 왼쪽 상세(제작 가능 여부, 최대 수량)를 함께 갱신
	OnOwnedMaterialsUpdated();
	OnSelectionUpdated();
}

void UCraftingWidget::CloseMenu()
{
	// ★ 구독 해제. 안 하면 위젯이 사라진 뒤에도 델리게이트가 남아
	// 이미 없는 위젯을 호출하려 할 수 있음.
	if (User && User->MaterialInventory)
	{
		User->MaterialInventory->OnMaterialChanged.RemoveDynamic(this, &UCraftingWidget::HandleMaterialChanged);
	}

	if (OwningAnvil)
	{
		OwningAnvil->CloseCraftingMenu();
	}
}