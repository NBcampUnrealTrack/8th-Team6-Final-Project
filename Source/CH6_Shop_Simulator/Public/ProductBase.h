// 매대에 진열되고 손님이 들고 갈 무기, 포션 상자들의 원조 부모 파일.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // 데이터 테이블 필수 인클루드 [1]
#include "GameFramework/Actor.h"
#include "ProductBase.generated.h"

// 상품 등급 정의
UENUM(BlueprintType)
enum class EProductRarity : uint8
{
	Normal   UMETA(DisplayName = "일반"),
	Rare     UMETA(DisplayName = "레어"),
	Epic     UMETA(DisplayName = "에픽"),
	Legendary UMETA(DisplayName = "전설")
};

// 🔥 [신설]: 스탯 상승 계산 방식 정의 (더하기 vs 곱하기)
UENUM(BlueprintType)
enum class EStatBonusType : uint8
{
	Flat       UMETA(DisplayName = "고정치 더하기 (+)"),
	Percentage UMETA(DisplayName = "비례 뻥튀기 (%)")
};

// ★ [데이터 테이블 행 구조체] (엑셀의 한 줄 정보) [1]
USTRUCT(BlueprintType)
struct FProductData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	FString ItemName; // 상품 이름

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	int32 BasePrice;  // 상품 기본 단가 (기본 가격)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product")
	EProductRarity Rarity; // 등급

	// 3D 메쉬가 아니라, 미리 만들어둔 '상품 블루프린트(BP)'를 통째로 등록하는 칸!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Asset")
	TSubclassOf<AProductBase> ProductBlueprintClass;

	// 🔥 [신설 스탯 기획 칸]: 이 아이템을 판매했을 때 스탯이 어떻게 오르는가?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Stat Bonus")
	EStatBonusType BonusType = EStatBonusType::Flat; // 기본값은 더하기

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Stat Bonus")
	float BonusValue = 5.0f; // 일반은 5, 레어는 10, 에픽은 3, 전설은 10 등등 적어둘 칸
};

// 1. 상품의 종류를 구별할 열거형(Enum) 정의
UENUM(BlueprintType)
enum class EProductType : uint8
{
	None         UMETA(DisplayName = "None"),
	Weapon       UMETA(DisplayName = "무기"),
	Armor        UMETA(DisplayName = "방어구"),
	HealthPotion UMETA(DisplayName = "체력 포션"),
	ManaPotion   UMETA(DisplayName = "마나 포션")
};

UCLASS()
class CH6_SHOP_SIMULATOR_API AProductBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProductBase();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 2. 이 상자가 어떤 종류의 상품인지 나타내는 변수 선언
	// EditAnywhere를 주었기 때문에 블루프린트 디테일 창에서 편하게 바꿀 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Info")
	EProductType ProductType = EProductType::None;

	// 🔥 [신설 스위치]: 이 상자가 현재 손님에게 낙점되어 들려있는 상태인가?
	// 기본값은 false(바닥이나 진열대에 평화롭게 있는 상태)입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Product State")
	bool bIsOwnedByCustomer = false;

	// 에디터에서 데이터 테이블 자체를 지정해 둡니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Settings")
	class UDataTable* ProductDataTable;

	// 이 상품의 고유 코드 ID (예: "Weapon_01", "Armor_05" 등 데이터 테이블의 행 이름)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Product Settings")
	FName ProductRowName;

	// 데이터 테이블에서 가져온 정보를 최종 계산해 보관할 변수들
	int32 Price = 0;
	FString ItemName;
	// 데이터 테이블에서 긁어와 보관할 버프 스탯 가방 변수들
	EStatBonusType MyBonusType = EStatBonusType::Flat;
	float MyBonusValue = 0.0f;

	// 데이터 테이블을 읽어서 가격을 세팅하는 함수
	void InitializeProductInfo();
};
