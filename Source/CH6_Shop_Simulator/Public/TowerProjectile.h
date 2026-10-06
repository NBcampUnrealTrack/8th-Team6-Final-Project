// Fill out your copyright notice in the Description page of Project Settings.
//
// 타워가 발사하는 탄환. 몬스터에 닿으면 데미지를 주고 스스로 파괴됨.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TowerProjectile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API ATowerProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATowerProjectile();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// ★ 실제로 눈에 보이는 탄환 모델. MeshComp의 자식이라 위치/이동은 그대로
	// 따라가지만, 이 컴포넌트 자신의 Relative Rotation은 코드가 절대 안 건드림.
	// MeshComp는 ProjectileMovementComponent의 bRotationFollowsVelocity 때문에
	// 매 프레임 "이동 방향"으로 회전이 강제 갱신되므로, 거기에 직접 회전 보정을
	// 넣어봐야 즉시 덮어써져서 소용없음 (화살처럼 길쭉한 모델이 옆으로 나가는
	// 문제가 여기서 생김). 화살/창 등 방향성 있는 모델은 이 컴포넌트에 넣고,
	// 여기 Relative Rotation을 수동으로 돌려서 보정하면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ProjectileVisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	// 얼마나 오래 살아있을지 (아무것도 안 맞고 계속 날아가는 경우를 대비한 안전장치)
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float LifeSpan = 3.f;

	float Damage = 10.f;

	// ★ 몬스터에 명중했을 때 확률적으로 띄울 텍스트.
	// 궁수처럼 연사가 빠른 타워는 확률을 낮게 잡을 것.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|FX")
	TSubclassOf<class AHitTextActor> HitTextClass;

	// 명중 시 텍스트가 뜰 확률 (1.0이면 항상, 0.2면 20퍼센트)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|FX", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitTextChance = 0.2f;

	// ★ 이 탄환으로 몬스터를 처치했을 때 띄울 텍스트.
	// 비워두면 처치 시에도 위의 HitTextClass를 그대로 씀.
	// 처치 전용 문구를 따로 쓰고 싶을 때만 지정하면 됨.
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|FX")
	TSubclassOf<class AHitTextActor> KillTextClass;

	// 처치했을 때는 항상 띄울지 (false면 위의 확률을 따름)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|FX")
	bool bAlwaysShowOnKill = true;

	// 텍스트가 뜨는 높이 (몬스터 위치 기준)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|FX")
	FVector HitTextOffset = FVector(0.f, 0.f, 120.f);

protected:
	// 명중 시 조건(확률, 처치 여부)을 따져서 타격 텍스트를 띄움
	void TrySpawnHitText(class AMonsterBase* Monster);

public:

	// ★ true면 타워가 이 탄환을 직선이 아니라 포물선으로 계산해서 발사함
	// (ATowerBase::FireAtTarget이 이 값을 보고 발사 방식을 다르게 함).
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	bool bUseArcTrajectory = false;

	// 포물선 높이 (0에 가까울수록 낮고 빠른 궤적, 1에 가까울수록 높이 붕 뜨는 궤적)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (EditCondition = "bUseArcTrajectory", ClampMin = "0.0", ClampMax = "2.0"))
	float ArcHeight = 0.5f;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	// 발사 직후 타워가 호출: 데미지 값과 속도(방향 포함)를 세팅
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitProjectile(float InDamage, const FVector& InVelocity);

	// 특정 액터를 이 탄환의 충돌 대상에서 제외시킴 (예: 기차 터렛을 쏘는 순간
	// 갑판 위에 서있는 플레이어 캐릭터 자신은 맞지 않게 하기 위함)
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void IgnoreActor(AActor* ActorToIgnore);

	bool UsesArcTrajectory() const { return bUseArcTrajectory; }
	float GetArcHeight() const { return ArcHeight; }
};