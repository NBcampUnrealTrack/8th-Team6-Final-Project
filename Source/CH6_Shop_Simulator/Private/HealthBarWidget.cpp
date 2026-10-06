// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthBarWidget.h"
#include "Components/ProgressBar.h"

void UHealthBarWidget::UpdateHealth(float Percent01)
{
	TargetPercent = FMath::Clamp(Percent01, 0.f, 1.f);

	// ★ 초록 바(앞)는 즉시 반영 - 다치면 바로 눈에 보여야 하니까.
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(TargetPercent);
	}

	// 하얀 잔상 바(뒤)는 그대로 두고, 타이머가 서서히 따라잡게 함.
	// 이미 애니메이션 중이면 목표치만 갱신하고 타이머는 계속 진행 (중복 시작 방지).
	if (DelayedHealthBar && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(AnimationTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			AnimationTimerHandle, this, &UHealthBarWidget::AnimateDelayedBar, 0.01f, true);
	}
}

void UHealthBarWidget::AnimateDelayedBar()
{
	if (!DelayedHealthBar)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(AnimationTimerHandle);
		}
		return;
	}

	// 매 스텝(0.02초)마다 목표치 쪽으로 천천히 다가감 (초록 바보다 느리게)
	DelayedDisplayPercent = FMath::FInterpTo(DelayedDisplayPercent, TargetPercent, 0.01f, DelayedBarSpeed);
	DelayedHealthBar->SetPercent(DelayedDisplayPercent);

	// 거의 다 따라잡았으면 정확히 스냅하고 타이머 정지 (여기서부턴 아무 연산도 안 함)
	if (FMath::IsNearlyEqual(DelayedDisplayPercent, TargetPercent, 0.001f))
	{
		DelayedDisplayPercent = TargetPercent;
		DelayedHealthBar->SetPercent(DelayedDisplayPercent);

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(AnimationTimerHandle);
		}
	}
}