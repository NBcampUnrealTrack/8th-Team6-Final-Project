// Fill out your copyright notice in the Description page of Project Settings.
//
// 보스 전용 체력바. 머리 위에 붙는 일반 체력바와 달리 화면 상단 중앙에
// 크게 표시됨. 지연 잔상 바 구조는 UHealthBarWidget과 동일하게 재사용하고,
// 보스 이름 표시와 피격 시 흔들림 연출을 추가함.
//
// UMG 구성:
// - Overlay 안에 DelayedHealthBar(하얀색, 뒤) -> HealthProgressBar(앞) 순서로 겹침
// - 그 위에 BossNameText 배치
// - 전체를 ShakeContainer(보통 Size Box나 Overlay)로 감싸면 피격 시 흔들림이 적용됨

#pragma once

#include "CoreMinimal.h"
#include "HealthBarWidget.h"
#include "BossHealthBarWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS()
class CH6_SHOP_SIMULATOR_API UBossHealthBarWidget : public UHealthBarWidget
{
	GENERATED_BODY()

public:
	// 보스가 등장할 때 호출. 이름을 세팅하고 체력바를 가득 채운 뒤 표시함.
	UFUNCTION(BlueprintCallable, Category = "Boss Health Bar")
	void ShowForBoss(const FText& BossName);

	// 보스가 죽거나 밤이 끝날 때 호출
	UFUNCTION(BlueprintCallable, Category = "Boss Health Bar")
	void HideBar();

	// 피격 시 체력바를 살짝 흔듦
	UFUNCTION(BlueprintCallable, Category = "Boss Health Bar")
	void PlayHitShake();

	// 흔들리는 세기 (픽셀 단위)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Health Bar")
	float ShakeIntensity = 8.f;

	// 흔들림이 지속되는 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Boss Health Bar")
	float ShakeDuration = 0.2f;

protected:
	// 보스 이름 - UMG에서 이 이름으로 만들 것
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;

	// 피격 시 흔들 대상. 체력바와 이름을 감싸는 컨테이너를 이 이름으로 만들면 됨.
	// 없으면 흔들림만 생략되고 나머지는 정상 작동.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ShakeContainer;

private:
	FTimerHandle ShakeTimerHandle;
	float ShakeRemaining = 0.f;

	void UpdateShake();
};