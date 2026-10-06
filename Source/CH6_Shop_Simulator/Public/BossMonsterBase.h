// Fill out your copyright notice in the Description page of Project Settings.
//
// 보스 몬스터 공통 베이스. AMonsterBase의 이동/전투 흐름은 그대로 물려받고,
// 보스만의 특수 능력 세 가지를 옵션으로 얹음.
//
// 1) 광역 공격  - 공격할 때 주변 방어시설/병사까지 같이 타격
// 2) 부하 소환  - 주기적으로 잡몹을 불러냄 (네크로맨서의 스켈레톤 등)
// 3) 주변 버프  - 근처 아군 몬스터의 공격력/이동속도를 올려줌
//
// 각 기능은 블루프린트에서 개별로 켜고 끌 수 있어서, 보스마다 다른 조합을
// 만들 수 있음. (예: 오크 족장 = 광역+버프, 네크로맨서 = 소환 중심)

#pragma once

#include "CoreMinimal.h"
#include "MonsterBase.h"
#include "BossMonsterBase.generated.h"

class UBossHealthBarWidget;

UCLASS()
class CH6_SHOP_SIMULATOR_API ABossMonsterBase : public AMonsterBase
{
	GENERATED_BODY()

public:
	ABossMonsterBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== 1) 광역 공격 =====

	// 켜면 공격 시 주 대상 외에 주변 것들도 같이 타격함
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Splash")
	bool bUseSplashAttack = true;

	// 광역 공격 반경
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Splash", meta = (EditCondition = "bUseSplashAttack"))
	float SplashRadius = 400.f;

	// 주 대상이 아닌 주변 것들이 받는 데미지 비율 (0.5면 절반)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Splash", meta = (EditCondition = "bUseSplashAttack"))
	float SplashDamageRatio = 0.6f;

	// ★ 광역 공격이 들어가는 순간 발생. 블루프린트에서 이 이벤트를 받아
	// 충격파 이펙트나 사운드를 붙이면 됨. 광역 반경도 같이 넘어가므로
	// 이펙트 크기를 맞출 때 참고할 수 있음.
	// (몽타주의 Anim Notify로 타이밍을 맞추고 싶으면 그쪽에서 처리해도 됨)
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Splash")
	void OnSplashAttack(const FVector& ImpactLocation, float Radius);

	// ===== 2) 부하 소환 =====

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	bool bUseSummon = false;

	// 소환할 몬스터 종류
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (EditCondition = "bUseSummon"))
	TSubclassOf<AMonsterBase> SummonClass;

	// 몇 초마다 소환할지
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (EditCondition = "bUseSummon"))
	float SummonInterval = 8.f;

	// 한 번에 몇 마리씩
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (EditCondition = "bUseSummon"))
	int32 SummonCountPerCast = 3;

	// ★ 동시에 유지할 부하 수 상한. 이걸 안 두면 보스가 오래 살아남을 때
	// 몬스터가 무한정 쌓여서 프레임이 무너짐.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (EditCondition = "bUseSummon"))
	int32 MaxAliveMinions = 20;

	// 보스 주변 어느 범위에 소환할지
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (EditCondition = "bUseSummon"))
	float SummonSpawnRadius = 300.f;

	// ===== 3) 주변 버프 =====

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff")
	bool bUseAuraBuff = false;

	// 버프가 닿는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	float BuffRadius = 800.f;

	// 공격력 배율 (1.3이면 30% 증가)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	float BuffDamageMultiplier = 1.3f;

	// 이동속도 배율
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	float BuffSpeedMultiplier = 1.2f;

	// 버프 갱신 주기 (매 프레임 할 필요 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	float BuffUpdateInterval = 1.f;

	// 발밑에 계속 떠있을 아우라 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	TObjectPtr<class UNiagaraSystem> AuraEffect;

	// ★ 이 이펙트가 "원래 크기(스케일 1)"에서 덮는 반경.
	// 코드가 BuffRadius / 이 값 만큼 이펙트를 확대해서 버프 범위와 크기를 맞춰줌.
	// 이펙트를 직접 열어 수정할 필요 없이 이 숫자만 맞추면 됨.
	// (예: 이펙트가 기본 상태로 반경 100 정도를 덮으면 100을 입력)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Buff", meta = (EditCondition = "bUseAuraBuff"))
	float AuraEffectBaseRadius = 100.f;

	// ★ 아우라 이펙트가 나올 위치. 블루프린트 뷰포트에서 눈으로 보며 옮기면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> AuraEffectPoint;

	// ===== 4) 도끼 던지기 (기차 견제) =====

	UPROPERTY(EditDefaultsOnly, Category = "Boss|AxeThrow")
	bool bUseAxeThrow = false;

	// 던질 도끼 투사체 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AxeThrow", meta = (EditCondition = "bUseAxeThrow"))
	TSubclassOf<class ABossAxeProjectile> AxeClass;

	// 몇 초마다 던질지
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AxeThrow", meta = (EditCondition = "bUseAxeThrow"))
	float AxeThrowInterval = 20.f;

	// 이 거리 안에 기차가 있을 때만 던짐 (너무 멀면 의미 없음)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AxeThrow", meta = (EditCondition = "bUseAxeThrow"))
	float AxeThrowRange = 3000.f;

	// 도끼가 나가는 위치 오프셋 (보스 발밑에서 나오면 어색하므로 살짝 위로)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|AxeThrow", meta = (EditCondition = "bUseAxeThrow"))
	FVector AxeSpawnOffset = FVector(0.f, 0.f, 150.f);

	// ★ 도끼를 던지는 순간 발생. 블루프린트에서 던지기 몽타주나 사운드를 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|AxeThrow")
	void OnAxeThrowStarted();

	// ===== 5) 전쟁의 함성 (궁극기) =====

	UPROPERTY(EditDefaultsOnly, Category = "Boss|WarCry")
	bool bUseWarCry = false;

	// 몇 초마다 발동할지
	UPROPERTY(EditDefaultsOnly, Category = "Boss|WarCry", meta = (EditCondition = "bUseWarCry"))
	float WarCryInterval = 40.f;

	// 타워와 플레이어가 무력화되는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|WarCry", meta = (EditCondition = "bUseWarCry"))
	float WarCryDuration = 3.f;

	// ★ 함성을 외치는 동안 보스가 제자리에 멈춰있는 시간(초).
	// 몽타주 길이에 맞춰두면 연출이 자연스러움.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|WarCry", meta = (EditCondition = "bUseWarCry"))
	float WarCryCastTime = 3.f;

	// 함성이 터질 때 나올 쇼크웨이브 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Boss|WarCry", meta = (EditCondition = "bUseWarCry"))
	TObjectPtr<class UNiagaraSystem> WarCryEffect;

	// ★ 이펙트가 나올 위치. 블루프린트 뷰포트에서 눈으로 보며 옮기면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> WarCryEffectPoint;

	// ★ 함성이 터지는 순간 발생. 블루프린트에서 몽타주, 사운드, 화면 이펙트를
	// 붙이면 됨 (이 연출이 없으면 플레이어가 왜 멈췄는지 알 수 없으므로 중요).
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|WarCry")
	void OnWarCryTriggered();

	// ===== 6) 원거리 포격 (네크로맨서 등) =====

	// ★ 켜면 근접 대신 원거리로 공격함. 사거리 안에 방어 대상이 들어오면
	// 그 자리에 멈춰서 마법구를 날림 (부하를 앞세우고 뒤에서 포격하는 그림).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|RangedAttack")
	bool bUseRangedAttack = false;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|RangedAttack", meta = (EditCondition = "bUseRangedAttack"))
	TSubclassOf<class ABossMagicProjectile> MagicProjectileClass;

	// 이 거리 안에 방어 대상이 들어오면 멈춰서 포격 시작
	UPROPERTY(EditDefaultsOnly, Category = "Boss|RangedAttack", meta = (EditCondition = "bUseRangedAttack"))
	float RangedAttackRange = 1500.f;

	// 포격 간격(초)
	UPROPERTY(EditDefaultsOnly, Category = "Boss|RangedAttack", meta = (EditCondition = "bUseRangedAttack"))
	float RangedAttackInterval = 3.f;

	// 마법구가 나가는 위치 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "Boss|RangedAttack", meta = (EditCondition = "bUseRangedAttack"))
	FVector MagicSpawnOffset = FVector(0.f, 0.f, 150.f);

	// ★ 포격하는 순간 발생. 블루프린트에서 시전 몽타주나 사운드를 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|RangedAttack")
	void OnRangedAttackFired();

	// ===== 보스 체력바 =====

	// 화면 상단에 띄울 보스 전용 체력바 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Boss|UI")
	TSubclassOf<class UBossHealthBarWidget> BossHealthBarClass;

	// 체력바에 표시될 보스 이름
	UPROPERTY(EditDefaultsOnly, Category = "Boss|UI")
	FText BossDisplayName = FText::FromString(TEXT("보스"));

	// ===== 디버그 =====

	// 광역/버프 범위를 화면에 원으로 표시 (밸런싱용)
	UPROPERTY(EditAnywhere, Category = "Boss|Debug")
	bool bShowRangeDebug = false;

	// ★ 이 보스를 잡으면 그 밤이 즉시 끝나는지. 켜두면 보스가 죽는 순간
	// 남아있던 몬스터가 전부 정리되고 다음 날로 넘어감.
	// (WaveManager가 매 체크마다 살아있는 보스가 있는지 확인해서 판단함)
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	bool bEndNightOnDeath = true;

protected:
	// 부모의 공격 처리에 광역 타격을 얹음
	virtual void PerformAttack() override;

	// 피격 시 보스 체력바를 갱신하고 흔들림 연출을 재생
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SummonMinions();
	void UpdateAuraBuff();
	void ThrowAxe();
	void TriggerWarCry();
	void EndWarCryCast();

	// 사거리 안의 방어 대상을 찾아 마법구를 날림
	void FireRangedAttack();

	// 사거리 안에서 가장 가까운 방어 대상을 찾음 (없으면 nullptr)
	class ADefenseObjectBase* FindRangedTarget() const;

	// 이 보스가 소환한 부하들 (상한 관리용)
	UPROPERTY()
	TArray<TObjectPtr<AMonsterBase>> SummonedMinions;

	FTimerHandle SummonTimerHandle;
	FTimerHandle AxeThrowTimerHandle;
	FTimerHandle WarCryTimerHandle;
	FTimerHandle WarCryCastTimerHandle;
	FTimerHandle RangedAttackTimerHandle;
	FTimerHandle CastEndTimerHandle;

	// ★ 스킬 몽타주를 재생하는 동안 켜지는 플래그. 부모의 bIsCasting은
	// 원거리 정지에도 쓰이므로, 시전 때문에 멈춘 건지 구분하기 위해 따로 둠.
	bool bIsCastingSkill = false;

	// 시전이 끝나 이동을 재개시킴
	void EndSkillCast();

	// 몽타주 길이만큼 이동을 멈춤 (몽타주가 없으면 기본 시간 사용)
	void StartSkillCast(float FallbackDuration);
	float BuffTimer = 0.f;

	// 화면에 떠있는 보스 체력바 인스턴스
	UPROPERTY()
	TObjectPtr<UBossHealthBarWidget> BossHealthBarInstance;

	// 발밑에 계속 떠있는 아우라 이펙트 (보스가 죽을 때 정리해야 하므로 들고 있음)
	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> ActiveAuraEffect;
};