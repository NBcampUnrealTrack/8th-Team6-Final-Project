// Fill out your copyright notice in the Description page of Project Settings.
//
// 4종류 선택창 (킹덤러시 타워 선택창 스타일). 처음부터 4칸 전부에 이름/아이콘/스탯/
// 느낌 텍스트가 다 보이고, 클릭하면 그 슬롯이 선택 상태(하이라이트)가 됨.
// OK 버튼으로 확정, None 버튼으로 취소.
//
// UMG 디자이너에서 할 일:
// - 슬롯 4개를 만들고, 각 슬롯 안에 이 이름들로 위젯을 배치:
//   TowerButton1~4(클릭 받는 버튼), TowerIcon1~4, TowerNameText1~4,
//   TowerStatsText1~4, TowerFlavorText1~4, TowerCostText1~4
// - (선택) 슬롯 전체를 감싸는 Border/Overlay를 SlotContainer1~4로 이름 지으면,
//   옵션이 없는 슬롯을 통째로 숨길 때 그 하나만 처리하면 됨
// - 각 버튼의 OnClicked 이벤트에 OnClickOption(인덱스) 연결
// - OK 버튼에 ConfirmSelection(), None 버튼에 CancelSelection() 연결
// - 이 클래스의 OnSelectionChanged(NewIndex, OldIndex) 이벤트를 오버라이드해서
//   선택된 슬롯 하이라이트 연출(테두리 밝기 등)을 블루프린트에서 구현

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuildTile.h"
#include "BuildSelectionWidget.generated.h"

class ABuildTile;
class UButton;
class UImage;
class UTextBlock;
class UWidget;

UCLASS()
class CH6_SHOP_SIMULATOR_API UBuildSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBuildSelectionWidget(const FObjectInitializer& ObjectInitializer);

	// ★ 이제 클릭 전에도 4칸 전부에 정보가 다 보여야 해서, Setup() 시점에
	// 아이콘뿐 아니라 이름/스탯/느낌 텍스트까지 슬롯마다 즉시 채워넣음.
	UFUNCTION(BlueprintCallable, Category = "Build Widget")
	void Setup(ABuildTile* InOwningTile, const TArray<FBuildOption>& InOptions);

	// 오른쪽 타워 버튼을 클릭했을 때 호출 (버튼의 OnClicked 이벤트에 연결).
	// "지금 선택된 것"으로 확정해두고, 하이라이트 갱신을 위해 OnSelectionChanged를 방송함.
	UFUNCTION(BlueprintCallable, Category = "Build Widget")
	void OnClickOption(int32 OptionIndex);

	// "OK!" 버튼에 연결. 지금 선택된 옵션으로 실제 건설 진행.
	UFUNCTION(BlueprintCallable, Category = "Build Widget")
	void ConfirmSelection();

	// "None!" 버튼(또는 Q키)에 연결. 아무것도 안 짓고 메뉴 닫기.
	UFUNCTION(BlueprintCallable, Category = "Build Widget")
	void CancelSelection();

	// ★ 선택 상태가 바뀔 때마다 호출됨 (OnClickOption 안에서). 블루프린트에서 이걸
	// 오버라이드해서, 선택된 슬롯은 밝게/테두리 강조하고 나머지는 원래대로 되돌리면 됨.
	// (NewIndex: 새로 선택된 슬롯, OldIndex: 이전에 선택되어 있던 슬롯 - 없었으면 -1)
	UFUNCTION(BlueprintImplementableEvent, Category = "Build Widget")
	void OnSelectionChanged(int32 NewIndex, int32 OldIndex);
	
protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeConstruct() override;

	// ===== 오른쪽 목록 (아이콘 버튼 4개) =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TowerButton1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TowerButton2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TowerButton3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TowerButton4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TowerIcon1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TowerIcon2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TowerIcon3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TowerIcon4;

	// ★ 이제 슬롯 안에 이름/스탯/느낌까지 처음부터 다 보여야 해서, 각 슬롯마다
	// 개별 텍스트블록을 둠 (킹덤러시 타워 선택창처럼). 왼쪽 공용 미리보기 패널
	// (PreviewXXX)은 이제 필수는 아니고, 안 쓰고 싶으면 그냥 안 만들면 됨.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerNameText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerNameText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerNameText3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerNameText4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerStatsText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerStatsText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerStatsText3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerStatsText4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerFlavorText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerFlavorText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerFlavorText3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerFlavorText4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerCostText1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerCostText2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerCostText3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TowerCostText4;

	// ★ 옵션: 버튼+아이콘+꾸밈 요소(액자, 반짝이 등)를 전부 하나로 감싸는 컨테이너
	// (Border, Overlay 등 아무 UWidget이나 가능). 이 이름으로 만들어두면, 안 쓰는
	// 슬롯일 때 이 컨테이너 하나만 Collapsed 처리해서 안의 모든 꾸밈 요소가
	// 한꺼번에 사라짐. 안 만들어도 되고, 그럴 땐 버튼/아이콘만 개별적으로 숨겨짐.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SlotContainer1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SlotContainer2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SlotContainer3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SlotContainer4;

	// 지금 선택된 인덱스. 블루프린트에서 하이라이트 로직 짤 때 참고용으로 읽을 수 있음.
	UPROPERTY(BlueprintReadOnly, Category = "Build Widget")
	int32 SelectedIndex = INDEX_NONE;
private:
	UPROPERTY()
	TObjectPtr<ABuildTile> OwningTile;

	TArray<FBuildOption> CachedOptions;

	
};