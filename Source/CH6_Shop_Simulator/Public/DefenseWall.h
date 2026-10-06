// Fill out your copyright notice in the Description page of Project Settings.
//
// 레벨에 미리 배치 + 비활성화 상태로 시작 -> 플레이어가 돈을 내면 Activate() 호출.
// 스탯(체력)은 BeginPlay가 아니라 Activate() 시점에 읽어와야
// 그 시점의 최신 FinalMaxHealth를 정확히 반영할 수 있음.

#pragma once

#include "CoreMinimal.h"
#include "DefenseObjectBase.h"
#include "DefenseWall.generated.h"

class USplineComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallDeactivated);

UCLASS()
class CH6_SHOP_SIMULATOR_API ADefenseWall : public ADefenseObjectBase
{
	GENERATED_BODY()
public:
	ADefenseWall();

	// 전투로 파괴되어(체력 0) Deactivate()될 때 브로드캐스트됨.
	// 이걸 지은 BuildTile이 이 신호를 받아서 "다시 빈 타일" 상태로 되돌리는 데 사용.
	UPROPERTY(BlueprintAssignable, Category = "Barricade")
	FOnWallDeactivated OnWallDeactivated;

protected:
	virtual void BeginPlay() override;

public:
	// 이 바리케이드가 막고 있는 몬스터 이동 스플라인을 "가진 액터"를 지정.
	// (스플라인 컴포넌트는 다른 액터 소속이라 컴포넌트 자체를 직접 고를 수 없어서 액터 단위로 참조)
	UPROPERTY(EditAnywhere, Category = "Barricade")
	AActor* KillZoneSplineActor;

	// 실제로 사용할 스플라인 컴포넌트 (BeginPlay에서 KillZoneSplineActor로부터 자동으로 찾아 캐싱)
	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> KillZoneSpline;

	// 스플라인 위 몇 유닛 지점인지. 숫자를 손으로 넣지 않고, 이 액터를 스플라인 경로 근처에
	// 배치해두면 BeginPlay에서 "가장 가까운 지점"을 찾아 자동으로 계산함.
	UPROPERTY(VisibleAnywhere, Category = "Barricade")
	float KillZoneDistance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Barricade")
	bool bIsActive = false;

	// 설치 비용 (플레이어 골드 차감은 호출부인 타일/PlayerController에서 처리)
	UPROPERTY(EditAnywhere, Category = "Barricade")
	int32 InstallCost = 200;

	// 타일에서 플레이어가 돈을 지불하고 설치할 때 호출
	UFUNCTION(BlueprintCallable, Category = "Barricade")
	void Activate();

	// 파괴되거나(체력 0) 플레이어가 스스로 철거할 때 호출 -> 원래의 "숨겨진 미배치" 상태로 복귀
	UFUNCTION(BlueprintCallable, Category = "Barricade")
	void Deactivate();

	// ADefenseObjectBase::TakeDamage를 오버라이드해서, 파괴 시 Destroy() 대신 Deactivate()가 불리게 함
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// 체력바 UI. 블루프린트에서 Widget Component를 추가한 다음(이름은 아무거나),
	// Construction Script나 BeginPlay에서 "Set Health Bar Widget Comp" 노드로
	// 그 컴포넌트 레퍼런스를 이 변수에 직접 연결해줘야 함 (자동 연결 안 됨).
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<class UWidgetComponent> HealthBarWidgetComp;

	// 이 거리 안에 플레이어가 있을 때만 체력바를 표시함
	UPROPERTY(EditAnywhere, Category = "UI")
	float HealthBarVisibleRange = 2400.f;

private:
	void UpdateHealthBar();
	void RefreshHealthBarVisibility();
	void CheckHealthBarPlayerDistance();

	FTimerHandle HealthBarVisibilityTimerHandle;
	bool bPlayerInHealthBarRange = false;
};