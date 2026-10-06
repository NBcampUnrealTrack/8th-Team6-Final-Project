// Fill out your copyright notice in the Description page of Project Settings.
//
// 큐브 메시로 대충 만든 타워. 사거리 안 몬스터를 감지 -> 포탑 머리를 회전시켜 조준 ->
// 주기적으로 탄환 발사. 메시는 나중에 교체해도 로직은 그대로 재사용 가능.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TowerBase.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AMonsterBase;
class ATowerProjectile;

UCLASS()
class CH6_SHOP_SIMULATOR_API ATowerBase : public AActor
{
	GENERATED_BODY()

public:
	ATowerBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ===== 컴포넌트 =====

	// 고정된 몸체(받침대). 회전 안 함.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMeshComp;

	// 몬스터를 조준하며 회전하는 포탑 머리 (=조준 방향을 결정하는 피벗).
	// 코드가 매 프레임 이 컴포넌트의 회전을 강제로 갱신하므로, 여기에 직접
	// 메시를 넣으면 수동으로 회전 보정을 할 수가 없음 (매번 덮어써짐).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretHeadComp;

	// ★ 실제로 눈에 보이는 포탑 메시. TurretHeadComp의 자식이라 회전을 그대로
	// 따라가지만, 이 컴포넌트 자신의 Relative Rotation은 코드가 안 건드림.
	// 모델링 파일의 "정면" 축이 언리얼 기준과 다르게 만들어져서 엉뚱한 방향을
	// 보고 있다면, 여기 Relative Rotation을 수동으로 돌려서 보정하면 됨
	// (예: Yaw 180을 넣으면 앞뒤가 뒤집힘).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretVisualMesh;

	// 탄환이 실제로 튀어나오는 위치 (포탑 머리 앞쪽에 붙여둠)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	// ===== 스탯 =====

	UPROPERTY(EditAnywhere, Category = "Tower | Stats")
	float Range = 800.f;

	// ★ 사거리를 화면에 원으로 표시. 평소엔 꺼져있다가, 플레이어가 이 타워를
	// 바라보면(InteractableComponent의 하이라이트가 켜지면) 자동으로 같이 켜짐.
	// 에디터에서 직접 true로 켜두면 항상 표시됨 (밸런싱 작업용).
	UPROPERTY(EditAnywhere, Category = "Tower|Debug")
	bool bShowRangeDebug = false;

	// ★ 일시적으로 공격 불가 상태. 보스의 "전쟁의 함성" 같은 스킬에 걸리면
	// 이 값이 켜지고, 그동안은 타겟이 있어도 발사하지 않음.
	UPROPERTY(BlueprintReadOnly, Category = "Tower")
	bool bIsDisabled = false;

	// 지정한 시간(초) 동안 이 타워를 공격 불가 상태로 만듦
	UFUNCTION(BlueprintCallable, Category = "Tower")
	void DisableForDuration(float Duration);

	// 무력화 중 재생할 몽타주 (스켈레탈 메시가 있는 타워만 해당)
	UPROPERTY(EditDefaultsOnly, Category = "Tower|Stun")
	TObjectPtr<class UAnimMontage> StunMontage;

	// 무력화 중 머리 위에 띄울 파티클 (별이 도는 효과 등)
	UPROPERTY(EditDefaultsOnly, Category = "Tower|Stun")
	TObjectPtr<class UNiagaraSystem> StunEffect;

	// ★ 파티클이 나올 위치. 블루프린트 뷰포트에서 눈으로 보며 옮기면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> StunEffectPoint;

protected:
	// ★ 이번 발사에 사용할 발사 위치. 기본은 MuzzlePoint 하나지만,
	// 궁수 타워처럼 여러 지점을 번갈아 쓰는 경우 자식에서 재정의하면 됨.
	virtual USceneComponent* GetActiveMuzzle() const;

	// 플레이어가 이 타워를 바라볼 때/시선을 뗄 때 호출됨 (사거리 표시 연동용)
	UFUNCTION()
	void HandleHighlightChanged(bool bHighlighted);

public:

	UPROPERTY(EditAnywhere, Category = "Tower | Stats")
	float FireRate = 1.f; // 몇 초마다 한 번 발사

	UPROPERTY(EditAnywhere, Category = "Tower | Stats")
	float Damage = 15.f;

	UPROPERTY(EditAnywhere, Category = "Tower | Stats")
	float ProjectileSpeed = 2000.f;

	// 회전 속도 (클수록 빠르게 조준함)
	UPROPERTY(EditAnywhere, Category = "Tower | Stats")
	float RotationInterpSpeed = 10.f;

	// 발사할 탄환 클래스 (에디터에서 BP_TowerProjectile 등을 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Tower | Stats")
	TSubclassOf<ATowerProjectile> ProjectileClass;

	// ===== 업그레이드 (모든 타워가 공통으로 가지는 범용 시스템) =====

	// 지금 몇 단계까지 업그레이드됐는지 (0 = 기본)
	UPROPERTY(BlueprintReadOnly, Category = "Tower | Upgrade")
	int32 UpgradeTier = 0;

	// 업그레이드 한 단계당 데미지/공속에 곱해질 배율
	UPROPERTY(EditAnywhere, Category = "Tower | Upgrade")
	float DamageMultiplierPerTier = 1.3f;

	UPROPERTY(EditAnywhere, Category = "Tower | Upgrade")
	float FireRateMultiplierPerTier = 0.85f; // 1보다 작아야 공속이 빨라짐 (FireRate = 발사 간격)

	// 업그레이드 실행. 기본 구현은 데미지/공속만 배율 적용.
	// 특정 타워만 업그레이드 시 특수 효과(사거리 증가, 관통 수 증가 등)를 추가하고 싶으면
	// 그 타워 전용 자식 클래스에서 이 함수를 오버라이드하면 됨 (base 호출 후 추가 로직만 덧붙이기).
	UFUNCTION(BlueprintCallable, Category = "Tower | Upgrade")
	virtual void ApplyUpgrade();

protected:
	// 몬스터 감지는 매 프레임이 아니라 주기적으로만 실행 (성능 부담 줄이기 위함)
	UPROPERTY(EditAnywhere, Category = "Tower | Performance")
	float DetectionInterval = 0.2f;

	// protected + virtual로 열어둠: 나중에 특정 타워만 고유한 타겟팅/발사 로직이
	// 필요해지면(예: 마법 타워의 특수 능력), 그 타워 전용 자식 클래스에서만
	// 오버라이드하면 됨. 그 외 타워는 지금처럼 그대로 base 구현을 씀.
	virtual void ScanForTarget();
	virtual void FireAtTarget();

	UPROPERTY()
	TObjectPtr<AMonsterBase> CurrentTarget;

	FTimerHandle DetectionTimerHandle;
	FTimerHandle DisableTimerHandle;

	// 지금 재생 중인 스턴 파티클 (해제될 때 꺼야 하므로 들고 있음)
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> ActiveStunEffect;

	void EndDisable();
	float FireCooldown = 0.f;
};