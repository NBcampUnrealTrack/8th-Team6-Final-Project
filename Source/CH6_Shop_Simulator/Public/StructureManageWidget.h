// Fill out your copyright notice in the Description page of Project Settings.
//
// 이미 지어진 구조물을 클릭했을 때 뜨는 관리 메뉴. BuildSelectionWidget과 완전히
// 같은 상호작용 패턴: 옵션(판매/업그레이드1/업그레이드2)을 클릭하면 "선택"만 되고
// (하이라이트 연출은 블루프린트가 OnSelectionChanged 받아서 처리), 확인 버튼을
// 눌러야 실제로 실행됨. 업그레이드 2칸은 아직 미구현이라 확인을 눌러도 실제
// 효과 없이 "준비 중" 피드백만 나감.
//
// 인덱스 규칙: 0 = 업그레이드1, 1 = 업그레이드2, 2 = 판매
//
// UMG 디자이너에서 할 일:
// - Sell Button, Upgrade1Button, Upgrade2Button 배치
// - 각 버튼의 OnClicked에 OnOptionClicked(0), OnOptionClicked(1), OnOptionClicked(2) 연결
// - 확인 버튼(건설 UI의 "OK!"와 동일한 역할)에 ConfirmSelection() 연결
// - 닫기/취소 버튼에 CancelMenu() 연결
// - Event On Selection Changed(New Index, Old Index)를 오버라이드해서 선택된
//   버튼 하이라이트(커짐 등) 연출 - 빌드 선택창에서 만든 것과 동일한 애니메이션 재사용 가능
// - Event On Upgrade Not Ready(UpgradeIndex)를 오버라이드해서 "준비 중" 피드백 연출

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StructureManageWidget.generated.h"

class ABuildTile;
class UTextBlock;

UCLASS()
class CH6_SHOP_SIMULATOR_API UStructureManageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UStructureManageWidget(const FObjectInitializer& ObjectInitializer);

	// BuildTile이 위젯을 띄운 직후 호출: 환불 예상 금액을 알려줌 (텍스트 표시용)
	UFUNCTION(BlueprintCallable, Category = "Structure Manage")
	void Setup(ABuildTile* InOwningTile, int32 InRefundAmount);

	// 옵션 버튼 클릭 시 호출 (0=판매, 1=업그레이드1, 2=업그레이드2).
	// 이 시점엔 아직 실행 안 되고 "선택"만 됨 - 확인 버튼을 눌러야 실제 실행.
	UFUNCTION(BlueprintCallable, Category = "Structure Manage")
	void OnOptionClicked(int32 OptionIndex);

	// 확인 버튼(빌드 UI의 "OK!"와 동일한 역할). 지금 선택된 옵션을 실제로 실행.
	UFUNCTION(BlueprintCallable, Category = "Structure Manage")
	void ConfirmSelection();

	// 취소/닫기 버튼에 연결
	UFUNCTION(BlueprintCallable, Category = "Structure Manage")
	void CancelMenu();

	// 선택 상태가 바뀔 때마다 호출됨. 블루프린트에서 오버라이드해서 선택된 버튼
	// 하이라이트, 이전 버튼 원복 연출을 구현하면 됨 (빌드 선택창과 동일한 패턴).
	UFUNCTION(BlueprintImplementableEvent, Category = "Structure Manage")
	void OnSelectionChanged(int32 NewIndex, int32 OldIndex);

	// 업그레이드(아직 미구현)를 확정 실행했을 때 블루프린트에서 받는 이벤트.
	// "준비 중입니다" 피드백 연출을 여기서 구현하면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Structure Manage")
	void OnUpgradeNotReady(int32 UpgradeIndex);

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeConstruct() override;

	// UMG 디자이너에서 텍스트블록을 만들고 이 이름으로 바인딩하면 "환불 XXG" 텍스트가 자동 채워짐
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SellText;

	// 지금 선택된 인덱스. 블루프린트에서 하이라이트 로직 짤 때 참고용으로 읽을 수 있음.
	UPROPERTY(BlueprintReadOnly, Category = "Structure Manage")
	int32 SelectedIndex = INDEX_NONE;

private:
	UPROPERTY()
	TObjectPtr<ABuildTile> OwningTile;
};