// Fill out your copyright notice in the Description page of Project Settings.
//
// 대장장이를 클릭하면 뜨는 제작 UI.
//
// 왼쪽에 선택한 레시피의 상세 정보(아이콘, 이름, 필요 재료, 수량 조절, 제작 버튼),
// 오른쪽에 만들 수 있는 목록이 뜨는 구조.
//
// 목록 버튼을 동적으로 만드는 부분은 블루프린트에서 처리하고,
// 여기서는 선택/수량/제작 같은 로직만 담당함.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CraftingTypes.h"
#include "CraftingWidget.generated.h"

class ACraftingAnvil;
class ACH6_Shop_SimulatorCharacter;

UCLASS()
class CH6_SHOP_SIMULATOR_API UCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 제작대가 UI를 띄울 때 호출. 어느 제작대와 누가 쓰는지 알려줌.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SetupForAnvil(ACraftingAnvil* InAnvil, ACH6_Shop_SimulatorCharacter* InUser);

	// ★ 목록에서 레시피를 클릭했을 때 호출. 수량은 1로 초기화됨.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void SelectRecipe(FName RecipeRowName);

	// 수량 조절 (화살표 버튼). Delta에 +1 또는 -1을 넣으면 됨.
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void ChangeCraftCount(int32 Delta);

	// 제작 버튼
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void ConfirmCraft();

	// 닫기 버튼
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CloseMenu();

	// ===== UI 갱신용 조회 함수 =====

	// 이 제작대에서 만들 수 있는 레시피 행 이름들 (목록 생성용)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FName> GetAvailableRecipes() const;

	// 행 이름으로 레시피 정보를 가져옴 (목록 버튼의 아이콘/이름 채울 때)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool GetRecipeData(FName RecipeRowName, FCraftRecipeData& OutData) const;

	// 지금 선택된 레시피
	UFUNCTION(BlueprintPure, Category = "Crafting")
	FName GetSelectedRecipe() const { return SelectedRecipeName; }

	// 지금 설정된 제작 수량
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetCraftCount() const { return CraftCount; }

	// ★ 지금 수량 기준으로 필요한 총 재료량. 상세 화면에 표시할 때 씀.
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FMaterialCost> GetCurrentTotalCosts() const;

	// 특정 재료의 보유 개수 (필요량과 비교해서 빨간색으로 표시할 때 씀)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetOwnedAmount(EMaterialType Type) const;

	// ★ 보유 중인 재료 전체. 제작 UI 한쪽에 "가진 재료" 목록을 띄울 때 씀.
	// 0개인 재료는 제외되므로 목록이 깔끔하게 유지됨.
	UFUNCTION(BlueprintPure, Category = "Crafting")
	TArray<FMaterialCost> GetOwnedMaterials() const;

	// 지금 수량으로 제작이 가능한지 (제작 버튼 활성화 판단)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraftNow() const;

	// ★ 제작대가 지금 작업 중인지. 작업 중이면 제작 버튼을 막아야 함.
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool IsAnvilBusy() const;

	// 아직 만들어야 할 개수 (진행 상황 표시용)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetRemainingCraftCount() const;

	// 이 레시피로 최대 몇 개까지 만들 수 있는지 (1 / 99 에서 뒤쪽 숫자)
	UFUNCTION(BlueprintPure, Category = "Crafting")
	int32 GetMaxCraftCount() const;

	// ★ 선택이나 수량이 바뀔 때마다 발생. 블루프린트에서 이걸 받아
	// 왼쪽 상세 화면을 다시 그리면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void OnSelectionUpdated();

	// ★ 제작대 정보가 들어온 뒤 발생. 오른쪽 목록을 여기서 만들면 됨.
	// (Event Construct는 아직 제작대가 연결되기 전이라 목록이 비어있음)
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void OnAnvilReady();

	// ★ 보유 재료가 바뀔 때 발생. 제작으로 재료를 소모했거나, 메뉴를 연 채로
	// 몬스터를 잡아 재료가 들어왔을 때 재료 목록을 다시 그리면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
	void OnOwnedMaterialsUpdated();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<ACraftingAnvil> OwningAnvil;

	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<ACH6_Shop_SimulatorCharacter> User;

private:
	FName SelectedRecipeName = NAME_None;
	int32 CraftCount = 1;

	// 재료 보유량이 바뀌었을 때 호출됨 (컴포넌트의 델리게이트를 구독)
	UFUNCTION()
	void HandleMaterialChanged(EMaterialType ChangedType, int32 NewAmount);
};