// Fill out your copyright notice in the Description page of Project Settings.
//
// 화면에 날짜와 시간을 표시. 낮에는 "09:00"처럼 시계로,
// 밤에는 "Wave 1 / 3"처럼 웨이브 진행도로 바뀜 (밤 길이가 웨이브 클리어로
// 결정되는 가변 길이라, 억지로 시계를 채우면 오히려 부자연스러움).
//
// UMG 구성 (둘 다 선택 사항 - 만든 것만 갱신됨):
// - DayText:  "Day 1"        (날짜 줄)
// - StatusText: "09:00" 또는 "Wave 1 / 3"  (그 아래 줄)
// - ClockText: 한 줄로 합쳐서 쓰고 싶을 때 (예전 방식)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameTimeManager.h"
#include "GameClockWidget.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API UGameClockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// AGameTimeManager가 시간이 바뀔 때마다 직접 호출해줌
	// CurrentWave/TotalWaves는 밤일 때만 쓰임 (낮엔 무시됨).
	// TotalWaves가 0이면 웨이브 표시 없이 "Night"만 나옴.
	UFUNCTION(BlueprintCallable, Category = "Game Clock")
	void UpdateDisplay(int32 Day, float Hour, EGamePhase Phase, int32 CurrentWave = 0, int32 TotalWaves = 0);

protected:
	// ★ 날짜 줄. "Day 1"
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DayText;

	// ★ 그 아래 줄. 낮이면 "09:00", 밤이면 "Wave 1 / 3"
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatusText;

	// 한 줄로 합쳐서 표시하고 싶을 때 쓰는 예전 방식 (안 만들어도 됨)
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* ClockText;
};