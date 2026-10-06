// 상품을 올려둘 상점 진열대와 계산대의 인프라 부모 파일.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProductBase.h" // ★ [필수] EProductType 열거형을 인식하기 위해 인클루드!
#include "DisplayBase.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API ADisplayBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADisplayBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	// ★ 진열대 메쉬 (기존엔 생성자 안 지역 변수였는데, 배치 모드에서 재질을
	// 바꿔치기하려면 밖에서 접근 가능한 멤버로 있어야 해서 이렇게 승격시킴)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	class UStaticMeshComponent* DisplayMesh;

	// ★ 실제 루트는 이 빈 컴포넌트가 담당. DisplayMesh를 직접 루트로 쓰면
	// C++ 네이티브 루트라서 블루프린트 에디터에서 위치를 자유롭게 못 옮기는
	// 문제가 있었음 (예전에 다른 클래스들에서도 겪었던 것과 동일한 원인).
	// DisplayMesh는 이 루트의 자식이라, 블루프린트에서 자유롭게 Relative
	// Location을 조정할 수 있음.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	class USceneComponent* DisplayRoot;

	// 1. 딱 하나의 상품만 안착할 눈에 안 보이는 '진열 위치 컴포넌트'
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	class USceneComponent* DisplaySlot;

	// 2. 현재 이 진열대에 상품이 올라가 있는지 저장하는 변수 (비어있으면 nullptr)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	AActor* OccupiedProduct;

	// 3. 이 진열대가 허용할 상품의 카테고리 (무기, 아머 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Settings")
	EProductType AcceptableProductType = EProductType::None;

	// 4. 자리가 비어있는지 직관적으로 확인하는 함수 (비어있으면 true)
	bool IsSlotEmpty() const { return OccupiedProduct == nullptr; }

	// 5. 손님이 물건 사러 와서 서 있을 발바닥 위치 (가구 정면 앞마당)
	// ★ 원래는 점 하나(USceneComponent)였는데, "이 자리에 다른 게 있으면 배치
	// 못 하게" 크기 검사를 하려면 크기 정보가 필요해서 UBoxComponent로 승격.
	// 블루프린트 뷰포트에서 박스 크기를 눈으로 보면서 진열대 종류마다 다르게
	// 조정 가능함 (콜리전 자체는 꺼둠 - 순수하게 "크기 정보"만 담는 용도).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	class UBoxComponent* CustomerWaitSlot;

	// ★ CustomerWaitSlot(=손님이 정확히 서는 지점)보다 넓게 잡는 "여유 공간" 확인용.
	// 배치할 때 이 박스 안에 다른 물체가 있으면 배치 불가로 처리함 (벽 구석처럼
	// 너무 좁은 곳에 손님이 끼는 걸 방지). CustomerWaitSlot과 같은 위치에 두되
	// 뷰포트에서 더 넓게/좁게 조정하면 됨.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display")
	class UBoxComponent* ClearanceZone;

	// 6. 이 진열대의 가격 버프 배율 (기본 진열대는 1.0 = 100%, 전설 진열대는 1.1 = 110%)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Settings")
	float PriceMultiplier = 1.0f; // ★ 에디터에서 전설 진열대는 이 값을 1.1f로 적어주면 +10%! ✅

	// 7. 🔥 상품이 입고되었을 때 주변 대기 손님들을 실시간으로 호출하는 알림 방송 기능
	void NotifyCustomersProductAvailable();

	// ===== ★ 구매 후 배치(고스트 모드) 관련 =====

	// 고스트 상태일 때 씌울 반투명 초록 재질
	UPROPERTY(EditDefaultsOnly, Category = "Placement")
	class UMaterialInterface* GhostMaterial;

	// 배치 불가능한 위치일 때 씌울 재질 (반투명 빨강 등)
	UPROPERTY(EditDefaultsOnly, Category = "Placement")
	class UMaterialInterface* InvalidGhostMaterial;

	// 고스트(미리보기) 모드 시작: 콜리전 끄고 반투명 재질로 전환
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void EnterGhostMode();

	// 배치 가능/불가능 여부에 따라 고스트 재질 색을 바꿈 (초록 <-> 빨강)
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void SetGhostValid(bool bIsValid);

	// 배치 확정: 콜리전 켜고 원래 재질로 복원 (진짜 진열대로 전환)
	UFUNCTION(BlueprintCallable, Category = "Placement")
	void ConfirmPlacement();

	bool IsInGhostMode() const { return bIsGhost; }

private:
	UPROPERTY()
	TArray<class UMaterialInterface*> OriginalMaterials;

	bool bIsGhost = false;
};