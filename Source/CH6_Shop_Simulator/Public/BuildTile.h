// Fill out your copyright notice in the Description page of Project Settings.
//
// 킬존/터렛 설치 타일. 레벨에 미리 배치해두고, 플레이어가 근처에서 좌클릭하면
// 4종류 선택 위젯을 띄움. 1~4번 키로 고르면 골드를 차감하고 실제 구조물을 설치함.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildTile.generated.h"

class UStaticMeshComponent;
class UBuildSelectionWidget;
class UStructureManageWidget;
class ACH6_Shop_SimulatorCharacter;

// 선택 가능한 항목 하나 (터렛 3종 + 바리케이드 1종 등)
USTRUCT(BlueprintType)
struct FBuildOption
{
	GENERATED_BODY()

	// 위젯에 표시할 이름 (예: "기본 터렛")
	UPROPERTY(EditAnywhere)
	FString Label;

	UPROPERTY(EditAnywhere)
	int32 Cost = 100;

	// 오른쪽 목록에 표시할 아이콘
	UPROPERTY(EditAnywhere)
	TObjectPtr<UTexture2D> Icon;

	// 왼쪽 미리보기 패널에 표시할 스탯 요약 (예: "공격력: 10\n공격속도: 상\n단일공격")
	UPROPERTY(EditAnywhere, meta = (MultiLine = "true"))
	FText StatsDescription;

	// 왼쪽 미리보기 패널에 표시할 느낌있는 한 줄 (예: "나의 화살을 무시하면 큰일날거야!")
	UPROPERTY(EditAnywhere)
	FText FlavorText;

	// 실제로 스폰할 액터 클래스 (타워든 바리케이드든 AActor 기반이면 다 가능)
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> StructureClass;

	// ★ 이 옵션을 이 타일에서 지을 때, 타일 자신의 회전에 추가로 더해줄 보정값.
	// 타일 본체의 Rotation은 안 건드리고, "이 타일에서 이 옵션을 지을 때만" 방향을
	// 조정하고 싶을 때 사용. 예: 서로 마주보는 두 타일이 같은 방향을 보고 있어서
	// 한쪽에서 병사 타워를 지으면 뒤돌아 서있는 경우, 그 타일의 이 옵션에만
	// Yaw 180을 넣어서 보정.
	UPROPERTY(EditAnywhere)
	FRotator SpawnRotationOffset = FRotator::ZeroRotator;
};

UCLASS()
class CH6_SHOP_SIMULATOR_API ABuildTile : public AActor
{
	GENERATED_BODY()

public:
	ABuildTile();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TileMesh;

	// ★ 구조물이 지어졌을 때 이 타일 자신의 메시를 숨길지 여부. 기본은 숨김(타워 등이
	// 타일 위에 겹쳐 보이는 걸 방지). 바리케이드 전용 타일처럼, 타일 메시 자체가
	// 구조물과 자연스럽게 같이 보여야 하는 경우엔 false로 꺼두면 됨.
	UPROPERTY(EditAnywhere, Category = "Build Tile")
	bool bHideTileMeshWhenBuilt = true;

	// 최대 4개까지 채워서 사용 (에디터에서 Label/Cost/StructureClass를 각각 지정)
	UPROPERTY(EditAnywhere, Category = "Build Tile")
	TArray<FBuildOption> Options;

	// 화면에 띄울 선택 위젯 블루프린트 (UBuildSelectionWidget을 부모로 만든 BP)
	UPROPERTY(EditDefaultsOnly, Category = "Build Tile")
	TSubclassOf<UBuildSelectionWidget> MenuWidgetClass;

	// 이미 지어진 타일을 클릭했을 때 띄울 관리(판매/업그레이드) 위젯
	UPROPERTY(EditDefaultsOnly, Category = "Build Tile")
	TSubclassOf<UStructureManageWidget> ManageWidgetClass;

	// 판매 시 지불했던 비용의 몇 %를 돌려줄지 (0.5 = 50% 환불)
	UPROPERTY(EditAnywhere, Category = "Build Tile")
	float SellRefundRatio = 0.5f;

	// 한 번 설치되면 다시 선택 못 하게 (바리케이드처럼 파괴 후 재설치를 원하면 이 값을 다시 false로)
	UPROPERTY(BlueprintReadOnly, Category = "Build Tile")
	bool bIsOccupied = false;

	// 지금 이 타일에 지어진 구조물 (없으면 nullptr)
	UPROPERTY(BlueprintReadOnly, Category = "Build Tile")
	TObjectPtr<AActor> BuiltStructure;

	// 지을 때 지불했던 비용 (판매 환불 계산용)
	UPROPERTY(BlueprintReadOnly, Category = "Build Tile")
	int32 PaidCost = 0;

	// ★ 짝꿍 타일. 이 값을 지정해두면, 이 타일에서 짓거나 팔 때 짝꿍 타일도 똑같이
	// 상태가 동기화됨 (둘 중 아무거나 클릭해도 같은 걸 짓고 파는 효과).
	// 예: 길목 양옆 두 타일이 가운데 바리케이드 하나를 같이 조작하게 만들 때 사용.
	UPROPERTY(EditInstanceOnly, Category = "Build Tile")
	TObjectPtr<ABuildTile> LinkedTile;

	// ★ 구조물을 스폰할 위치를 이 타일 자신의 위치가 아니라 다른 곳으로 지정하고 싶을 때 사용.
	// (예: 양옆 타일에서 조작하지만 실제 바리케이드는 가운데 길목에 생겨야 하는 경우)
	// 비워두면 원래대로 이 타일의 위치에 스폰됨.
	UPROPERTY(EditInstanceOnly, Category = "Build Tile")
	TObjectPtr<AActor> SpawnLocationOverride;

	// 플레이어가 이 타일을 좌클릭했을 때 호출 (캐릭터의 상호작용 함수에서 불러줄 것)
	// 내부에서 빈 타일이면 건설 메뉴, 지어진 타일이면 관리 메뉴를 알아서 띄움.
	UFUNCTION(BlueprintCallable, Category = "Build Tile")
	void TryOpenMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter);

	// 건설 위젯에서 1~4번 키를 누르면 호출됨
	UFUNCTION(BlueprintCallable, Category = "Build Tile")
	void OnOptionSelected(int32 OptionIndex);

	// 관리 위젯에서 "판매" 선택 시 호출됨
	UFUNCTION(BlueprintCallable, Category = "Build Tile")
	void OnSellSelected();

	// ADefenseWall이 전투로 파괴(Deactivate)됐을 때 알려주는 델리게이트에 구독시킬 함수.
	// 타일을 다시 빈 상태로 되돌려서 재설치 메뉴가 뜨게 함.
	UFUNCTION()
	void HandleStructureDestroyed();

	// ★ 저장 데이터에서 불러올 때, 돈을 내지 않고 구조물만 바로 세움.
	// (일반 건설 흐름은 골드 차감과 UI를 거치므로 로드에는 맞지 않음)
	UFUNCTION(BlueprintCallable, Category = "Build Tile")
	void RestoreStructure(UClass* StructureClass, int32 InPaidCost);

private:
	// 지금 저장 데이터를 복원하는 중인지. 켜져 있으면 골드 차감과
	// UI/입력 처리를 건너뜀.
	bool bIsRestoringFromSave = false;

public:

	// 위젯에서 ESC를 누르면 호출됨 - 아무것도 안 하고 그냥 메뉴 닫기
	UFUNCTION(BlueprintCallable, Category = "Build Tile")
	void OnMenuCancelled();

	// ★ 건설이 완료된 순간 발생. 블루프린트에서 이 이벤트를 받아 파티클과
	// 사운드를 붙이면 됨 (연출을 코드에 박아두지 않고 블루프린트에서 자유롭게
	// 교체할 수 있도록 이벤트만 열어둠).
	UFUNCTION(BlueprintImplementableEvent, Category = "Build Tile|FX")
	void OnStructureBuilt(const FVector& BuildLocation);

	// ★ 판매(철거)가 완료된 순간 발생.
	UFUNCTION(BlueprintImplementableEvent, Category = "Build Tile|FX")
	void OnStructureSold(const FVector& SellLocation);

private:
	void OpenBuildMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter);
	void OpenManageMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter);

	// 메뉴가 열려있는 동안 이동/시점 회전을 막고, 닫히면 복구
	void FreezePlayerInput();
	void UnfreezePlayerInput();

	UPROPERTY()
	TObjectPtr<UBuildSelectionWidget> ActiveBuildWidget;

	UPROPERTY()
	TObjectPtr<UStructureManageWidget> ActiveManageWidget;

	UPROPERTY()
	TObjectPtr<ACH6_Shop_SimulatorCharacter> PendingInstigator;
};