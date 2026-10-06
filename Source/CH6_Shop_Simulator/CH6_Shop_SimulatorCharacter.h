// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProductBase.h"
#include "CraftingAnvil.h"
#include "CH6_Shop_SimulatorCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UPhysicsHandleComponent;
class UShopPlacementComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class ACH6_Shop_SimulatorCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	// 마우스 왼쪽 클릭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractAction_LeftClick;

	// 마우스 오른쪽 클릭 (배치 모드 취소용으로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CancelAction_RightClick;

	// ★ 진열대 구매 후 배치(고스트 모드) 담당 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop | Placement", meta = (AllowPrivateAccess = "true"))
	class UShopPlacementComponent* ShopPlacementComp;
public:
	ACH6_Shop_SimulatorCharacter();

	// ★ [통장 잔고 💵]: 플레이어가 현재 가지고 있는 누적 골드 변수
	// BlueprintReadOnly를 주었기 때문에 UI 블루프린트가 이 값을 실시간으로 읽어갈 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy")
	int32 PlayerGold = 1000; // 시작 자금

	// 1. [고정 스탯] 일반, 레어를 팔면 누적되는 순수 고정 공격력 (기본값 10)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	int32 BaseAttackPower = 10;

	// 2. [퍼센트 배율] 에픽, 전설을 팔면 누적되는 배율 (기본값 1.0f = 100%)
	// 에픽(3%)을 팔면 1.03f가 되고, 전설(10%)을 팔면 1.13f가 됩니다. (단순 합산 방식)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	float AttackMultiplier = 1.0f;

	// 3. [최종 공격력] 밤에 터렛들이 가져갈 진짜 최종 실시간 공격력!
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	int32 FinalAttackPower = 10;

	// 최대 체력도 똑같이 3분할로 세팅해 둡니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	int32 BaseMaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	float HealthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop Economy | Stats")
	int32 FinalMaxHealth = 100;

	// ★ 고정과 퍼센트가 바뀔 때마다 최종 공격력을 실시간으로 갱신해 주는 함수
	void UpdateFinalStats();

	// 플레이어가 타워 빌딩 타일을 클릭했을때 호출되는 함수
	bool TryOpenBuildTile();

	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	bool bIsInteractionLocked = false;

	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	class ABuildTile* CurrentOpenBuildTile = nullptr;

	// ★ 지금 플레이어가 바라보고 있는 상호작용 대상. 없으면 nullptr.
	// UI 위젯에서 이 값을 읽어서 "좌클릭으로 상호작용" 문구를 띄우면 됨
	// (있으면 표시, 없으면 숨김 / 문구는 GetInteractionText()로 가져오기).
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	class UInteractableComponent* CurrentInteractable = nullptr;

	// 상호작용 가능 거리 (라인트레이스 길이)
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	// 대상 탐색 주기. 매 프레임 할 필요는 없고, 0.1초면 체감상 즉각적임.
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionCheckInterval = 0.1f;

	// ★ 보스의 "전쟁의 함성" 등에 걸리면 이동이 크게 느려지고 시야가 좁아짐.
	// 완전히 못 움직이게 하면 답답하므로, 느려지는 정도로만 제약을 둠.
	UFUNCTION(BlueprintCallable, Category = "Player")
	void ApplyStun(float Duration);

	// 스턴 중인지 (UI나 애니메이션에서 참고 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	bool bIsStunned = false;

	// 스턴 중 이동 속도 배율 (0.2면 평소의 20% 속도)
	UPROPERTY(EditAnywhere, Category = "Player")
	float StunSpeedMultiplier = 0.2f;

	// 스턴 중에 시야가 좁아지는 정도 (기본 FOV에서 이만큼 뺌)
	UPROPERTY(EditAnywhere, Category = "Player")
	float StunFOVReduction = 10.f;

	// FOV가 변하는 속도 (클수록 빠르게 전환)
	UPROPERTY(EditAnywhere, Category = "Player")
	float StunFOVInterpSpeed = 4.f;

	// ★ 계산된 최종 스탯을 실제 액터들에 반영.
	// 무기 판매 -> 기차 터렛 공격력, 방어구 판매 -> 베이스 코어 최대 체력.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ApplyStatsToWorld();

	// ★ 재료 보유량 관리. 몬스터 드랍으로 쌓이고, 제작할 때 소모됨.
	// 시작 재료는 이 컴포넌트의 StartingMaterials에서 지정.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<class UMaterialInventoryComponent> MaterialInventory;

	virtual void BeginPlay() override;

	// 가게 오픈 사인
	bool TryOpenSign();

	// 기차에 빙의
	bool TryMountTrain();

	// ★ 진열대 구매 UI에서 "구매 확정" 눌렀을 때 호출. 골드 체크+차감 후
	// 배치(고스트) 모드를 시작함. 골드가 부족하면 false 반환하고 아무 일도 안 일어남.
	// SpawnLocation: 처음 스폰될 안전한 지점 (도면의 ShelfSpawnPoint 등, 주변과 안 겹치는 곳)
	UFUNCTION(BlueprintCallable, Category = "Shop | Placement")
	bool BuyAndPlaceDisplay(TSubclassOf<class ADisplayBase> DisplayClass, int32 Cost, FVector SpawnLocation);

	// ★ 불가능한 행동을 시도했을 때(골드 부족, 상품 있는 진열대 재배치 시도 등)
	// 재사용할 카메라 흔들림 피드백. 에디터에서 원하는 카메라 셰이크 블루프린트 지정.
	UPROPERTY(EditAnywhere, Category = "Shop | Feedback")
	TSubclassOf<class UCameraShakeBase> RejectionCameraShake;

	// 위 카메라 셰이크를 실제로 재생하는 함수 (어디서든 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "Shop | Feedback")
	void PlayRejectionShake();

	// 진열대 구매 UI("도면")를 클릭했을 때
	bool TryOpenShopBuildStation();

	// 이미 지어진 진열대를 클릭했을 때 (다시 집어서 옮기기)
	bool TryPickupExistingDisplay();

	UShopPlacementComponent* GetShopPlacementComp() const { return ShopPlacementComp; }

	// ShopPlacementComponent가 진열대를 물리적으로 들고 다니는 데 재사용
	class UPhysicsHandleComponent* GetPhysicsHandleComp() const { return PhysicsHandleComp; }
protected:
	// 마우스 오른쪽 클릭 시 발동 (배치 모드 중이면 취소+환불)
	void OnCancelPressed();

	// ★ 앞을 보고 상호작용 대상을 찾아서 외곽선을 켜고 끔.
	// Tick에서 InteractionCheckInterval 주기로 호출됨.
	void UpdateInteractionTarget();

private:
	float InteractionCheckTimer = 0.f;

	// 스턴 해제용 타이머와 FOV 관련 값
	FTimerHandle StunTimerHandle;
	float OriginalFOV = 90.f;
	float TargetFOV = 0.f; // 0이면 아직 스턴을 겪지 않은 상태 (보간 안 함)
	float OriginalWalkSpeed = 0.f;
	void EndStun();

protected:
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	void Tick(float DeltaTime);
protected:


	// ★ [내 손에 물건이 들려있는지 감시하는 스위치 전구] 💡
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop | Interaction")
	bool bIsHoldingProduct = false;

	// ★ [우주 최강 무적의 자석 핸들 부품] 🔑
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UPhysicsHandleComponent* PhysicsHandleComp;

	// 마우스 왼쪽 클릭 시 발동되는 메인 상호작용 입구
	void OnInteractPressed();
private:
	// 🔥 [역할 분담 비서 함수 3형제 신설]
	/** 1. 손에 물건을 들고 있을 때: 진열대에 상자를 배치하는 기능 */
	void TryPlaceProductOnDisplay();

	/** 2. 빈손일 때: 바닥에 떨어진 물건 상자를 집어 올리는 기능 */
	void TryPickupProductFromWorld();

	/** 3. 빈손일 때: 계산대 맨 앞의 손님을 클릭해 결제 정산하는 기능 */
	bool TryCheckoutCustomer();
public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }



};