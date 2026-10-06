// Fill out your copyright notice in the Description page of Project Settings.
//
// 진열대 구매 UI. 옵션이 딱 2개(무기 진열대/방어구 진열대)뿐이라, 타워 선택창처럼
// "선택 후 확인" 단계 없이 버튼 클릭 한 번으로 바로 구매+배치 모드 시작.
//
// UMG 디자이너에서 할 일:
// - BuyWeaponButton, BuyArmorButton 배치
// - 각 버튼의 OnClicked에 OnBuyWeaponClicked() / OnBuyArmorClicked() 연결
// - 닫기 버튼에 CloseMenu() 연결

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopBuildWidget.generated.h"

class ADisplayBase;
class ACH6_Shop_SimulatorCharacter;
class AShopBuildStation;

UCLASS()
class CH6_SHOP_SIMULATOR_API UShopBuildWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShopBuildWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Shop Build")
	void Setup(ACH6_Shop_SimulatorCharacter* InCharacter, AShopBuildStation* InStation);

	UFUNCTION(BlueprintCallable, Category = "Shop Build")
	void OnBuyWeaponClicked();

	UFUNCTION(BlueprintCallable, Category = "Shop Build")
	void OnBuyArmorClicked();

	UFUNCTION(BlueprintCallable, Category = "Shop Build")
	void CloseMenu();

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeConstruct() override;

	// 무기 진열대 클래스/가격 (에디터에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Shop Build")
	TSubclassOf<ADisplayBase> WeaponShelfClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop Build")
	int32 WeaponShelfCost = 500;

	UPROPERTY(EditDefaultsOnly, Category = "Shop Build")
	TSubclassOf<ADisplayBase> ArmorShelfClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop Build")
	int32 ArmorShelfCost = 500;

private:
	UPROPERTY()
	TObjectPtr<ACH6_Shop_SimulatorCharacter> OwningCharacter;

	UPROPERTY()
	TObjectPtr<AShopBuildStation> OwningStation;

	void TryBuy(TSubclassOf<ADisplayBase> DisplayClass, int32 Cost);
};