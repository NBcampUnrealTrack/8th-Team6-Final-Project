// Fill out your copyright notice in the Description page of Project Settings.

#include "BossHealthBarWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UBossHealthBarWidget::ShowForBoss(const FText& BossName)
{
	if (BossNameText)
	{
		BossNameText->SetText(BossName);
	}

	UpdateHealth(1.f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBossHealthBarWidget::HideBar()
{
	GetWorld()->GetTimerManager().ClearTimer(ShakeTimerHandle);

	// 흔들림 중이었다면 원위치로 되돌림
	if (ShakeContainer)
	{
		ShakeContainer->SetRenderTranslation(FVector2D::ZeroVector);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UBossHealthBarWidget::PlayHitShake()
{
	if (!ShakeContainer || ShakeDuration <= 0.f)
	{
		return;
	}

	ShakeRemaining = ShakeDuration;

	// ★ 이미 흔들리는 중이면 타이머를 새로 걸지 않고 시간만 갱신.
	// (연타로 맞을 때 타이머가 중복으로 쌓이는 것 방지)
	if (!GetWorld()->GetTimerManager().IsTimerActive(ShakeTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(ShakeTimerHandle, this,
			&UBossHealthBarWidget::UpdateShake, 0.02f, true);
	}
}

void UBossHealthBarWidget::UpdateShake()
{
	ShakeRemaining -= 0.02f;

	if (ShakeRemaining <= 0.f || !ShakeContainer)
	{
		// 흔들림이 끝나면 원위치로 되돌리고 타이머 정지
		if (ShakeContainer)
		{
			ShakeContainer->SetRenderTranslation(FVector2D::ZeroVector);
		}
		GetWorld()->GetTimerManager().ClearTimer(ShakeTimerHandle);
		return;
	}

	// 남은 시간에 비례해 흔들림이 점점 잦아들게 함
	const float Ratio = ShakeRemaining / ShakeDuration;
	const float CurrentIntensity = ShakeIntensity * Ratio;

	const FVector2D Offset(
		FMath::RandRange(-CurrentIntensity, CurrentIntensity),
		FMath::RandRange(-CurrentIntensity, CurrentIntensity));

	ShakeContainer->SetRenderTranslation(Offset);
}