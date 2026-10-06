// Fill out your copyright notice in the Description page of Project Settings.
//
// 제작대(대장장이). 클릭하면 제작 UI가 열리고, 거기서 만들 것과 수량을 고르면
// 재료를 소모해서 상품을 만들어냄.
//
// 예전에는 클릭 즉시 확률로 랜덤 등급이 나왔는데, 플레이어가 원하는 걸
// 만들 수 있도록 선택 제작 방식으로 바꿈.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CraftingTypes.h"
#include "CraftingAnvil.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API ACraftingAnvil : public AActor
{
	GENERATED_BODY()

public:
	ACraftingAnvil();

	// 대장장이 메시 (루트). 애니메이션을 쓸 수 있도록 스켈레탈 메시로 둠.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anvil")
	class USkeletalMeshComponent* AnvilMesh;

	// 제작된 상품이 나타날 위치. 여기서 스폰돼서 물리로 아래에 떨어짐.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anvil")
	class USceneComponent* ProductSpawnSlot;

	// 상품 정보가 들어있는 데이터 테이블 (DT_ProductPrices)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anvil Settings")
	class UDataTable* ProductDataTable;

	// ★ 이 제작대에서 만들 수 있는 레시피들 (FCraftRecipeData 테이블).
	// 무기 대장장이와 방어구 대장장이가 서로 다른 테이블을 쓰면 됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anvil Settings")
	class UDataTable* RecipeDataTable;

	// 클릭했을 때 열릴 제작 UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anvil Settings")
	TSubclassOf<class UCraftingWidget> CraftingWidgetClass;

	// ★ 상품 하나를 만드는 데 걸리는 시간(초).
	// 여러 개를 주문하면 이 간격으로 하나씩 나옴.
	// (한꺼번에 쏟아지면 물리 연산도 부담이고 바닥이 상품으로 뒤덮임)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anvil Settings")
	float CraftTimePerItem = 3.f;

	// 지금 제작 중인지 (UI에서 진행 상황을 보여줄 때 씀)
	UFUNCTION(BlueprintPure, Category = "Anvil")
	bool IsCrafting() const { return RemainingCraftCount > 0; }

	// 아직 만들어야 할 개수
	UFUNCTION(BlueprintPure, Category = "Anvil")
	int32 GetRemainingCraftCount() const { return RemainingCraftCount; }

	// ★ 제작 UI를 염. 플레이어가 클릭하면 호출됨.
	UFUNCTION(BlueprintCallable, Category = "Anvil")
	void OpenCraftingMenu(class ACH6_Shop_SimulatorCharacter* InUser);

	// 제작 UI를 닫음
	UFUNCTION(BlueprintCallable, Category = "Anvil")
	void CloseCraftingMenu();

	// ★ 실제 제작. 재료를 소모하고 상품을 Count개 만들어냄.
	// 재료가 부족하면 아무것도 안 하고 false를 반환.
	UFUNCTION(BlueprintCallable, Category = "Anvil")
	bool CraftProduct(FName RecipeRowName, int32 Count);

	// 제작 성공 시 발생. 블루프린트에서 이펙트나 사운드를 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Anvil")
	void OnProductCrafted(int32 CraftedCount);

	// ★ 상품이 하나 나올 때마다 발생 (망치질 소리, 불꽃 이펙트 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "Anvil")
	void OnSingleItemForged();

	// 주문한 수량을 전부 만들었을 때 발생
	UFUNCTION(BlueprintImplementableEvent, Category = "Anvil")
	void OnCraftingFinished();

	// 이 제작대의 레시피 행 이름 목록 (UI가 목록을 채울 때 사용)
	UFUNCTION(BlueprintPure, Category = "Anvil")
	TArray<FName> GetRecipeRowNames() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 지금 이 제작대를 쓰고 있는 플레이어
	UPROPERTY()
	class ACH6_Shop_SimulatorCharacter* CurrentUser;

	// 화면에 떠있는 제작 UI
	UPROPERTY()
	class UCraftingWidget* ActiveWidget;

	// 상품 하나를 스폰 (제작 수량만큼 반복 호출됨)
	void SpawnProduct(FName ProductRowName);

	// ★ 시간차 제작 진행 상태.
	// 재료는 주문할 때 한 번에 소모하고, 상품만 천천히 나옴.
	FTimerHandle CraftTimerHandle;
	FName CraftingRecipeName = NAME_None;
	int32 RemainingCraftCount = 0;

	// 타이머가 호출. 상품 하나를 만들고 남은 수량을 줄임.
	void ForgeNextItem();
};