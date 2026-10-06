// Fill out your copyright notice in the Description page of Project Settings.
//
// 몬스터/병사 머리 위에 떠있는 체력바. UWidgetComponent에 붙여서 씀.
// 데미지를 받거나 회복할 때만 갱신하면 되고, 매 프레임 갱신할 필요 없음
// (그래서 Tick이 아니라 TakeDamage/회복 시점에서 직접 호출하는 방식).
//
// 구조 (다른 게임에서 흔히 보이는 "잔상 체력바" 연출):
// - 앞쪽 초록색 바(HealthProgressBar)는 데미지를 받으면 즉시 깎임
// - 그 뒤에 있는 하얀색 바(DelayedHealthBar)는 천천히 따라 떨어짐 (얼마나 깎였는지 잔상으로 보여줌)
// UMG에서 Overlay 안에 DelayedHealthBar(하얀색, 뒤) -> HealthProgressBar(초록색, 앞) 순서로 겹쳐야 함.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class CH6_SHOP_SIMULATOR_API UHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 0.0 ~ 1.0 사이 비율로 체력바 채움. 초록 바는 즉시 반영되고, 하얀 잔상
	// 바만 천천히 뒤따라감.
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void UpdateHealth(float Percent01);

	// 하얀 잔상 바가 목표치를 따라잡는 속도 (작을수록 더 천천히, 오래 잔상이 남음)
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float DelayedBarSpeed = 1.5f;

protected:
	// 앞쪽 초록색 바 - UMG에서 이 이름으로 만들 것
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthProgressBar;

	// 뒤쪽 하얀색 잔상 바 - UMG에서 이 이름으로 만들고, HealthProgressBar보다
	// Overlay 안에서 먼저(=더 뒤에) 배치할 것. 안 만들어도 에러 없이 그냥 잔상 효과만 없어짐.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> DelayedHealthBar;

private:
	float TargetPercent = 1.f;

	// 하얀 잔상 바가 지금 보여주고 있는 값. 이것만 타이머로 서서히 좁혀나감
	// (초록 바는 UpdateHealth에서 바로 반영되므로 별도 변수가 필요 없음).
	float DelayedDisplayPercent = 1.f;

	// ★ 목표치에 도달할 때까지만 도는 타이머. Tick과 달리, 애니메이션이 끝나면
	// 스스로 멈춰서 그 이후엔 아무 연산도 안 함 (체력바가 여러 개 떠있어도 부담 없음).
	FTimerHandle AnimationTimerHandle;
	void AnimateDelayedBar();
};