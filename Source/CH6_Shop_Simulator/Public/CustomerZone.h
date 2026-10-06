// Fill out your copyright notice in the Description page of Project Settings.
//
// 손님 통행로의 각 지점을 레벨에 직접 배치해서 지정하는 마커.
// 좌표를 코드에 박아두면 상점 위치를 조금만 옮겨도 전부 어긋나기 때문에,
// AShelfSpawnZone과 같은 방식으로 레벨에서 눈으로 보며 잡을 수 있게 함.
//
// 배치 방법:
// 1. 손님이 나타나는 자리(A구역)에 하나 놓고 Zone Type을 Spawn으로
// 2. 가게 앞 길목에 하나 놓고 Zone Type을 ShopEntrance로 (박스 범위 = 감지 구역)
// 3. 손님이 사라지는 자리(B구역)에 하나 놓고 Zone Type을 Exit으로

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomerZone.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class ECustomerZoneType : uint8
{
	Spawn         UMETA(DisplayName = "스폰 지점 (A구역)"),
	ShopEntrance  UMETA(DisplayName = "가게 감지 구역 (길목 중간)"),
	Exit          UMETA(DisplayName = "퇴장 지점 (B구역)")
};

UCLASS()
class CH6_SHOP_SIMULATOR_API ACustomerZone : public AActor
{
	GENERATED_BODY()

public:
	ACustomerZone();

	// 레벨 뷰포트에서 이 박스의 크기/위치를 직접 조정해서 구역을 잡으면 됨
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> ZoneVolume;

	// 이 마커가 어떤 역할인지
	UPROPERTY(EditAnywhere, Category = "Customer Zone")
	ECustomerZoneType ZoneType = ECustomerZoneType::Spawn;

	// 박스 범위 안에서 랜덤한 월드 좌표 하나를 반환.
	// 손님마다 조금씩 다른 자리에서 나타나거나 사라지게 해서 자연스럽게 만듦.
	UFUNCTION(BlueprintCallable, Category = "Customer Zone")
	FVector GetRandomPoint() const;

	// 특정 위치가 이 구역 박스 안에 들어와 있는지
	UFUNCTION(BlueprintCallable, Category = "Customer Zone")
	bool IsLocationInside(const FVector& WorldLocation) const;

	// 레벨에서 지정한 타입의 구역을 찾아서 반환 (없으면 nullptr)
	static ACustomerZone* FindZone(const UObject* WorldContext, ECustomerZoneType InType);
};