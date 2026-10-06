// Fill out your copyright notice in the Description page of Project Settings.
//
// ACharacter -> APawn으로 변경.
// CharacterMovementComponent 대신 USplineFollowComponent로 이동.
// 공격 대상은 전부 ADefenseObjectBase*로 통일 (BaseCore, DefenseWall 등 모두 이 타입).
// -> 몬스터는 "지금 코어를 공격 중인지 바리케이드를 공격 중인지" 구분할 필요 없이
//    ADefenseObjectBase::TakeDamage()만 호출하면 됨.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SplineFollowComponent.h"
#include "MonsterBase.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USplineFollowComponent;
class USplineComponent;
class ADefenseObjectBase;
class UAnimMontage;

// ACustomerAI의 ECustomerState와 같은 패턴 - 프로젝트 컨벤션 유지
UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	Moving,      // 스플라인을 따라 이동 중
	Attacking,   // 방어시설(코어 or 바리케이드) 앞에서 정지, 공격 중
	Dead
};

// ★ 울프 라이더처럼 한 몬스터에 스켈레탈 메시가 여러 개인 경우, 각 메시마다
// 다른 몽타주를 동시에 재생하기 위한 짝. 예: 늑대 메시 + 늑대 물기 몽타주,
// 오크 메시 + 오크 창찌르기 몽타주
USTRUCT(BlueprintType)
struct FExtraMeshMontage
{
	GENERATED_BODY()

	// 몽타주를 재생할 스켈레탈 메시 컴포넌트의 이름
	// (블루프린트 컴포넌트 목록에 보이는 이름 그대로 입력)
	UPROPERTY(EditAnywhere, Category = "Extra Mesh")
	FName MeshComponentName;

	UPROPERTY(EditAnywhere, Category = "Extra Mesh")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Extra Mesh")
	TObjectPtr<UAnimMontage> DeathMontage;
};

UCLASS()
class CH6_SHOP_SIMULATOR_API AMonsterBase : public APawn
{
	GENERATED_BODY()

public:
	AMonsterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ===== 컴포넌트 =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineFollowComponent> SplineFollowComp;

	// ===== 스탯 =====

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster | Health")
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster | Health")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster | Attack")
	float AttackDamage = 10.f;

	// ★ 이 몬스터가 죽을 때 줄 재료를 정의한 데이터 테이블 (FMonsterDropData).
	// 비워두면 아무것도 안 줌.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Drop")
	TObjectPtr<class UDataTable> DropTable;

	// 그 테이블에서 이 몬스터가 참조할 행 이름들.
	// 여러 개를 넣으면 각각 따로 확률 판정을 함 (철은 100%, 금은 20% 식으로).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Drop")
	TArray<FName> DropRowNames;

	// ★ 보스의 아우라 버프를 이미 받았는지. 버프는 스탯에 곱하기로 적용되므로,
	// 이 플래그가 없으면 주기마다 계속 곱해져서 무한히 강해짐.
	UPROPERTY(BlueprintReadOnly, Category = "Monster")
	bool bHasBossBuff = false;

	// ★ 스킬 시전 중이면 이동을 멈춤. 보스가 궁극기 몽타주를 재생하는 동안
	// 계속 걸어가면 연출이 어색해지므로, 그동안 제자리에 서있게 함.
	UPROPERTY(BlueprintReadWrite, Category = "Monster")
	bool bIsCasting = false;

	// ★ 근접 타격 시 확률적으로 띄울 텍스트 ("POW!", "SOK!" 등).
	// 비워두면 아무것도 안 뜸.
	UPROPERTY(EditDefaultsOnly, Category = "Monster|FX")
	TSubclassOf<class AHitTextActor> HitTextClass;

	// 텍스트가 뜰 확률 (1.0이면 항상, 0.25면 25퍼센트)
	UPROPERTY(EditDefaultsOnly, Category = "Monster|FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitTextChance = 0.25f;

	// 텍스트가 뜨는 높이 (대상 위치 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Monster|FX")
	FVector HitTextOffset = FVector(0.f, 0.f, 120.f);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster | Attack")
	float AttackInterval = 1.f;

	// ===== 상태 =====

	UPROPERTY(BlueprintReadOnly, Category = "Monster | State")
	EMonsterState CurrentState = EMonsterState::Moving;

	// 현재 공격 중인 대상. BaseCore일 수도, DefenseWall(바리케이드)일 수도 있음.
	// -> 타입을 ADefenseObjectBase*로 통일했기 때문에 몬스터 로직은 둘을 구분하지 않음.
	UPROPERTY(BlueprintReadOnly, Category = "Monster | State")
	TObjectPtr<ADefenseObjectBase> CurrentTarget = nullptr;

	int32 CurrentSegmentIndex = INDEX_NONE;

	// ===== 외부 호출용 =====

	// 스폰 매니저가 몬스터 생성 직후 호출
	UFUNCTION(BlueprintCallable, Category = "Monster")
	void InitializeMonster(USplineComponent* InSpline, float InHealthMultiplier, float InDamageMultiplier);

	// 둔화 (터렛 효과)
	UFUNCTION(BlueprintCallable, Category = "Monster")
	void ApplySlow(float SlowRatio, float Duration);

	// 기절 (터렛 효과)
	UFUNCTION(BlueprintCallable, Category = "Monster")
	void ApplyStun(float Duration);

	// 몬스터가 데미지를 받을 때 (플레이어 터렛/기차 터렛에서 호출)
	// ADefenseObjectBase와 마찬가지로 표준 TakeDamage 시그니처를 따름
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator, AActor* DamageCauser) override;

	// ===== AnimBP 연동용 헬퍼 =====

	// AnimBP에서 Walk/Idle 판단용. 실제로 멈춰있는 상황에서는 0을 반환.
	// ★ bIsCasting(보스 스킬 시전 중)일 때도 제자리에 서 있으므로 0을 반환해야
	// Idle 모션이 나옴. 안 그러면 멈춰서 걷는 모션이 계속 재생됨.
	UFUNCTION(BlueprintCallable, Category = "Monster")
	float GetCurrentSpeed() const
	{
		if (CurrentState != EMonsterState::Moving || bIsCasting || !SplineFollowComp)
		{
			return 0.f;
		}
		return SplineFollowComp->BaseSpeed * SplineFollowComp->SpeedMultiplier;
	}

	// AnimBP State Machine의 "공격 상태로 전환" 조건에 바로 연결해서 쓸 수 있음
	UFUNCTION(BlueprintCallable, Category = "Monster")
	bool IsAttacking() const { return CurrentState == EMonsterState::Attacking; }

	// ★ 외부(병사 등)에서 이 몬스터를 강제로 멈춰 세우고 특정 대상과 싸우게 만듦.
	// 물리적 충돌(콜리전)에 의존하지 않고 직접 상태를 전환하는 방식이라, 콜리전
	// 설정이 어떻든 상관없이 100% 확실하게 작동함. Attacking 상태가 되는 즉시
	// TickMoving이 더 이상 안 불리니 이동도 자동으로 멈춤.
	UFUNCTION(BlueprintCallable, Category = "Monster")
	void ForceEngage(ADefenseObjectBase* Target)
	{
		if (CurrentState == EMonsterState::Dead || !Target)
		{
			return;
		}
		CurrentTarget = Target;
		CurrentState = EMonsterState::Attacking;
		AttackTimer = 0.f;
	}

protected:
	// ★ 실제 타격 처리. 기본은 현재 대상 하나만 때리지만, 보스처럼 광역 공격이
	// 필요한 경우 자식 클래스에서 이 함수를 오버라이드하면 됨.
	virtual void PerformAttack();

private:
	void TickMoving(float DeltaTime);
	void TickAttacking(float DeltaTime);

	void Die();
	void FinishDying();

	// 죽을 때 드랍 테이블을 보고 플레이어에게 재료를 지급
	void GiveDropMaterials();

	// 체력바 UI (몬스터 머리 위). Widget Class는 이 컴포넌트 자체의 디테일 패널에서
	// 지정 (UWidgetComponent가 이미 그 항목을 내장하고 있어서 따로 프로퍼티 안 만듦).
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<class UWidgetComponent> HealthBarWidgetComp;

	// 이 거리 안에 플레이어가 있을 때만 체력바를 표시함
	UPROPERTY(EditAnywhere, Category = "UI")
	float HealthBarVisibleRange = 2400.f;

	// 매 프레임 거리 계산할 필요 없어서 이 간격으로만 체크
	float HealthBarVisibilityCheckTimer = 0.f;
	static constexpr float HealthBarVisibilityCheckInterval = 0.3f;

	// 플레이어가 지금 범위 안에 있는지 (거리 체크 결과를 저장해둠)
	bool bPlayerInHealthBarRange = false;

	// 체력이 바뀔 때마다(데미지 받을 때) 이 함수를 호출해서 위젯 갱신
	void UpdateHealthBar();

	// "범위 안에 있음" + "체력이 풀피가 아님" 두 조건을 합쳐서 실제 표시 여부 결정
	void RefreshHealthBarVisibility();

	// 사망 시 재생할 몽타주. 재생이 끝날 때까지 기다렸다가 Destroy()함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage;

	// 몽타주가 끝나기 이 시간(초)만큼 전에 미리 파괴함. 몽타주 끝자락의 Blend Out
	// 구간(죽은 포즈 -> 원래 Idle 포즈로 되돌아가는 보간) 때문에 "다시 일어나는"
	// 것처럼 보이는 어색함을 방지하기 위함.
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	float DeathMontageEndBuffer = 0.15f;

	// ★ 메인 메시(MeshComp) 외에 추가로 몽타주를 재생할 메시들.
	// 울프 라이더처럼 늑대 메시 + 오크 메시가 따로 있는 경우, 각 메시마다
	// 다른 몽타주를 동시에 재생해야 해서 이 목록에 등록함.
	// (메인 메시는 여기 넣지 말 것 - 위의 AttackMontage/DeathMontage로 이미 재생됨)
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Combat", meta = (AllowPrivateAccess = "true"))
	TArray<FExtraMeshMontage> ExtraMeshMontages;

protected:
	// 공격 시 재생할 몽타주. 블루프린트 클래스 기본값으로 몬스터 종류별로 지정.
	// (보스가 시전 시간 계산에 쓰므로 자식에서 접근 가능해야 함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 메인 메시와 추가 메시들 전부에 공격 몽타주를 재생
	// (보스의 원거리 공격처럼 자식 클래스에서도 쓰므로 protected)
	void PlayAttackMontageOnAllMeshes();

private:
	// 메인 메시와 추가 메시들 전부에 사망 몽타주를 재생. 가장 긴 재생 시간을 반환.
	float PlayDeathMontageOnAllMeshes();

	// 이름으로 스켈레탈 메시 컴포넌트를 찾음 (추가 메시 몽타주용)
	USkeletalMeshComponent* FindMeshByName(FName ComponentName) const;

	// 메인/추가 메시 중 하나라도 공격 몽타주가 재생 중인지
	bool IsAnyAttackMontagePlaying() const;

	bool bIsStunned = false;
	float AttackTimer = 0.f;

	FTimerHandle StunTimerHandle;
	FTimerHandle SlowTimerHandle;
	FTimerHandle DeathTimerHandle;
};