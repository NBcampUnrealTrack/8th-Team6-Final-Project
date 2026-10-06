// Fill out your copyright notice in the Description page of Project Settings.

#include "GameClockWidget.h"
#include "Components/TextBlock.h"

void UGameClockWidget::UpdateDisplay(int32 Day, float Hour, EGamePhase Phase, int32 CurrentWave, int32 TotalWaves)
{
	// ===== 아랫줄에 들어갈 내용 먼저 결정 =====
	// 낮이면 시계, 밤이면 웨이브 진행도.
	FString StatusString;

	if (Phase == EGamePhase::Day)
	{
		const int32 HourPart = FMath::FloorToInt(Hour);
		const int32 MinutePart = FMath::FloorToInt((Hour - HourPart) * 60.f);
		StatusString = FString::Printf(TEXT("%02d:%02d"), HourPart, MinutePart);
	}
	else
	{
		// 밤은 길이가 웨이브 클리어 여부로 정해지는 가변 길이라 숫자 시계 대신
		// 웨이브 진행 상황을 표시함. 웨이브 정보가 아직 없으면 Night만.
		StatusString = (TotalWaves > 0)
			? FString::Printf(TEXT("Wave %d / %d"), CurrentWave, TotalWaves)
			: FString(TEXT("Night"));
	}

	// ===== 두 줄로 나눠 쓰는 경우 =====
	if (DayText)
	{
		DayText->SetText(FText::FromString(FString::Printf(TEXT("Day %d"), Day)));
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(StatusString));
	}

	// ===== 한 줄로 합쳐 쓰는 경우 (예전 방식) =====
	if (ClockText)
	{
		ClockText->SetText(FText::FromString(
			FString::Printf(TEXT("Day %d · %s"), Day, *StatusString)));
	}
}