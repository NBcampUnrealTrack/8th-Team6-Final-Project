// Fill out your copyright notice in the Description page of Project Settings.
//
// 플레이어가 지켜야 하는 최종 목표물. 파괴되면 게임 오버.
// 낮에 방어구를 팔아 올린 최종 체력을 그대로 받아 씀.

#pragma once

#include "CoreMinimal.h"
#include "DefenseObjectBase.h"
#include "BaseCore.generated.h"

// 코어 체력이 바뀔 때 HUD에 알려주는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoreHealthChanged, int32, NewHealth, int32, NewMaxHealth);

/**
 *
 */
UCLASS()
class CH6_SHOP_SIMULATOR_API ABaseCore : public ADefenseObjectBase
{
	GENERATED_BODY()
public:
	ABaseCore();

	// ★ 체력이 바뀔 때마다 발생. HUD 위젯이 이걸 구독해서 체력바를 갱신하면 됨.
	// (매 프레임 값을 읽어가는 것보다 가볍고, 갱신 시점도 정확함)
	UPROPERTY(BlueprintAssignable, Category = "Base Core")
	FOnCoreHealthChanged OnCoreHealthChanged;

	// 현재 체력 (HUD에서 처음 표시할 때 사용)
	UFUNCTION(BlueprintPure, Category = "Base Core")
	int32 GetCoreHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Base Core")
	int32 GetCoreMaxHealth() const { return MaxHealth; }

	// 체력 비율 (0~1). 프로그레스 바에 바로 넣으면 됨.
	UFUNCTION(BlueprintPure, Category = "Base Core")
	float GetCoreHealthPercent() const
	{
		return (MaxHealth > 0) ? (static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)) : 0.f;
	}

	// ★ 레벨의 베이스 코어를 찾아서 반환. HUD 위젯처럼 참조가 없는 곳에서
	// 코어를 찾을 때 쓰면 됨.
	UFUNCTION(BlueprintCallable, Category = "Base Core", meta = (WorldContext = "WorldContext"))
	static ABaseCore* GetBaseCore(const UObject* WorldContext);

	// ★ 체력 값이 바뀌었음을 알림. 외부에서 MaxHealth를 직접 바꾼 경우
	// (방어구 판매로 최대 체력이 오를 때) 호출해주면 HUD가 갱신됨.
	UFUNCTION(BlueprintCallable, Category = "Base Core")
	void NotifyHealthChanged();

	// ★ 파괴되면 게임 오버 처리. 부모는 체력이 0이 되면 Destroy()하는데,
	// 코어는 사라지기 전에 게임을 끝내야 하므로 여기서 가로챔.
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	// 코어가 파괴되는 순간 발생. 블루프린트에서 폭발 이펙트나 사운드를 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Base Core")
	void OnCoreDestroyed();

protected:
	virtual void BeginPlay() override;

private:
	// 게임 오버가 중복으로 호출되지 않도록
	bool bHasTriggeredGameOver = false;
};