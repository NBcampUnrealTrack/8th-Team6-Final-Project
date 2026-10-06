// Fill out your copyright notice in the Description page of Project Settings.
//
// 타워가 소환하는 병사. ADefenseObjectBase를 상속받아서 체력/TakeDamage/파괴 로직을 그대로 재사용.
// 몬스터와 물리적으로 부딪혀서 막고(Sweep), 서로 근접 데미지를 주고받는 1대1 전투.

#pragma once

#include "CoreMinimal.h"
#include "DefenseObjectBase.h"
#include "SoldierUnit.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class AMonsterBase;
class UAnimMontage;

UCLASS()
class CH6_SHOP_SIMULATOR_API ASoldierUnit : public ADefenseObjectBase
{
	GENERATED_BODY()

public:
	ASoldierUnit();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	// 이 사거리 안에 들어오면 멈춰서 공격 시작 (감지 자체는 이제 ASoldierTower가 담당)
	UPROPERTY(EditAnywhere, Category = "Soldier | Combat")
	float MeleeRange = 150.f;

	UPROPERTY(EditAnywhere, Category = "Soldier | Combat")
	float AttackDamage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Soldier | Combat")
	float AttackInterval = 1.f;

	// 사거리 밖의 몬스터를 감지하면 이 속도로 다가감
	UPROPERTY(EditAnywhere, Category = "Soldier | Combat")
	float MoveSpeed = 300.f;

	// 전투 중이 아닐 때(복귀 중이거나 대기 중일 때) 초당 회복량
	UPROPERTY(EditAnywhere, Category = "Soldier | Combat")
	float HealthRegenPerSecond = 5.f;

	// 원래 위치(스폰된 곳). 몬스터를 다 처치하면 다시 이 자리로 돌아옴
	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	FVector HomeLocation;

	// 원래 바라보던 방향. 복귀하면 이 방향으로도 다시 맞춰줌
	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	FRotator HomeRotation;

	// AnimBP에서 Idle/Walk 판단용. 몬스터에게 다가가는 중이면 MoveSpeed, 멈춰서
	// 싸우거나 대기 중이면 0을 반환.
	UFUNCTION(BlueprintCallable, Category = "Soldier")
	float GetCurrentSpeed() const { return bIsMovingToTarget ? MoveSpeed : 0.f; }

	// ★ 타워(ASoldierTower)가 여러 병사를 보고 "누가 누굴 때릴지" 중앙에서
	// 정해서 배정. 병사 자신은 더 이상 스스로 몬스터를 찾지 않음.
	UFUNCTION(BlueprintCallable, Category = "Soldier")
	void SetAssignedTarget(AMonsterBase* NewTarget);

	// 상대할 몬스터가 없을 때 타워가 호출 (원래 자리로 복귀시킴)
	UFUNCTION(BlueprintCallable, Category = "Soldier")
	void ClearAssignedTarget() { CurrentTarget = nullptr; }

	// 지금 배정받은 대상 조회 (타워가 "이미 싸우는 중인지" 확인할 때 사용)
	AMonsterBase* GetCurrentTarget() const { return CurrentTarget; }

	// 사망 몽타주 재생 중인지 (몬스터가 "이 대상은 더 이상 반격 못 하는 상태"를
	// 확인할 때 사용 - 몽타주 끝나고 실제 Destroy되기 전까지도 true)
	bool IsDying() const { return bIsDying; }

	// ★ 근접 타격 시 확률적으로 띄울 텍스트 ("POW!", "SOK!" 등).
	// 비워두면 아무것도 안 뜸.
	UPROPERTY(EditDefaultsOnly, Category = "Soldier|FX")
	TSubclassOf<class AHitTextActor> HitTextClass;

	// 텍스트가 뜰 확률 (1.0이면 항상, 0.25면 25퍼센트)
	UPROPERTY(EditDefaultsOnly, Category = "Soldier|FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitTextChance = 0.25f;

	// 텍스트가 뜨는 높이 (대상 위치 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Soldier|FX")
	FVector HitTextOffset = FVector(0.f, 0.f, 120.f);


	// ★ 보스의 "전쟁의 함성" 등으로 일시적으로 공격 불가 상태가 됨.
	// 그동안은 목표가 있어도 공격하지 않고 제자리에 멈춰 있음.
	UFUNCTION(BlueprintCallable, Category = "Soldier")
	void DisableForDuration(float Duration);

	UPROPERTY(BlueprintReadOnly, Category = "Soldier")
	bool bIsDisabled = false;

	// 무력화 중 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Soldier|Stun")
	TObjectPtr<class UAnimMontage> StunMontage;

	// 무력화 중 머리 위에 띄울 파티클
	UPROPERTY(EditDefaultsOnly, Category = "Soldier|Stun")
	TObjectPtr<class UNiagaraSystem> StunEffect;

	// ★ 파티클이 나올 위치. 블루프린트 뷰포트에서 눈으로 보며 옮기면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> StunEffectPoint;

	// 데미지를 받아 체력이 0 이하가 되면 사망 몽타주를 재생하고, 끝나면 파괴함
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

private:
	UPROPERTY()
	TObjectPtr<AMonsterBase> CurrentTarget;

	float AttackTimer = 0.f;

	// 제자리(전투 중 아님)에 있을 때 체력 회복 처리
	void RegenerateHealth(float DeltaTime);

	void Die();
	void FinishDying();

	// 체력바 UI (병사 머리 위). Widget Class는 컴포넌트 자체 디테일에서 지정.
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<class UWidgetComponent> HealthBarWidgetComp;

	// 이 거리 안에 플레이어가 있을 때만 체력바를 표시함
	UPROPERTY(EditAnywhere, Category = "UI")
	float HealthBarVisibleRange = 2400.f;

	void UpdateHealthBar();

	// "범위 안에 있음" + "체력이 풀피가 아님" 두 조건을 합쳐서 실제 표시 여부 결정
	void RefreshHealthBarVisibility();

	// 매 프레임 거리 계산할 필요 없어서 이 간격으로만 체크
	float HealthBarVisibilityCheckTimer = 0.f;
	static constexpr float HealthBarVisibilityCheckInterval = 0.3f;

	// 플레이어가 지금 범위 안에 있는지 (거리 체크 결과를 저장해둠)
	bool bPlayerInHealthBarRange = false;

	// 지금 이 순간 무엇을 해야 하는지에 따라 나눈 하위 로직들. Tick()은 이 중
	// 뭘 부를지만 판단하고, 실제 동작은 각 함수가 담당함.
	bool IsWaitingForAttackMontage() const;
	void HandleReturnHome(float DeltaTime);
	void HandleCombat(float DeltaTime);

	// 공격 시 재생할 몽타주. AMonsterBase와 같은 패턴.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> AttackMontage;

	// 사망 시 재생할 몽타주. 재생이 끝날 때까지 기다렸다가 Destroy()함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage;

	// 몽타주가 끝나기 이 시간(초)만큼 전에 미리 파괴함. 몽타주 끝자락의 Blend Out
	// 구간(죽은 포즈 -> 원래 Idle 포즈로 되돌아가는 보간) 때문에 "다시 일어나는"
	// 것처럼 보이는 어색함을 방지하기 위함.
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	float DeathMontageEndBuffer = 0.15f;

	FTimerHandle DeathTimerHandle;

	// 사망 몽타주 재생 중에는 Tick의 이동/전투 로직이 끼어들지 않게 막는 플래그
	bool bIsDying = false;

	// 무력화 해제용 타이머와 재생 중인 파티클
	FTimerHandle DisableTimerHandle;

	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> ActiveStunEffect;

	void EndDisable();

	// GetCurrentSpeed()가 참조하는 상태 - Tick에서 매 프레임 갱신됨
	bool bIsMovingToTarget = false;

	// "막혀서 못 움직이는 상태"를 감지하기 위한 변수들
	FVector LastTickLocation = FVector::ZeroVector;
	float StuckTimer = 0.f;

	// 체력 재생 시 소수점 회복량이 정수 CurrentHealth에서 버려지지 않도록 누적
	float RegenAccumulator = 0.f;
};