// Fill out your copyright notice in the Description page of Project Settings.
//
// 진열대 구매 UI를 여는 상호작용 지점("도면"). OpenSignActor와 같은 패턴 -
// 플레이어가 상호작용하면 구매 UI를 띄움.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopBuildStation.generated.h"

class UStaticMeshComponent;
class UShopBuildWidget;
class ACH6_Shop_SimulatorCharacter;

UCLASS()
class CH6_SHOP_SIMULATOR_API AShopBuildStation : public AActor
{
	GENERATED_BODY()

public:
	AShopBuildStation();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	// ★ 진열대가 처음 스폰될 구역. BP 안에 고정된 상대 위치 대신, 레벨에 직접
	// 배치한 AShelfSpawnZone을 여기서 지정함 ("가게 바로 앞" 등 원하는 자리를
	// 레벨 뷰포트에서 눈으로 보면서 잡을 수 있음). 각 도면 인스턴스마다 다른
	// 구역을 지정할 수도 있음.
	UPROPERTY(EditInstanceOnly, Category = "Shop Build Station")
	TObjectPtr<class AShelfSpawnZone> SpawnZone;

	// 뜨울 구매 UI 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Shop Build Station")
	TSubclassOf<UShopBuildWidget> BuildWidgetClass;

	// 캐릭터의 상호작용 함수에서 호출
	UFUNCTION(BlueprintCallable, Category = "Shop Build Station")
	void TryOpen(ACH6_Shop_SimulatorCharacter* InteractingCharacter);

private:
	UPROPERTY()
	TObjectPtr<UShopBuildWidget> ActiveWidget;
};