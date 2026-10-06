// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCore.h"
#include "CH6_Shop_Simulator/CH6_Shop_SimulatorCharacter.h"
#include "GameTimeManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ABaseCore::ABaseCore()
{
}

void ABaseCore::BeginPlay()
{
	Super::BeginPlay();

	// 🎯 [낮 영업 연동]: 플레이어가 낮에 열심히 방어구 팔아 올린 최종 체력을 내 맷집으로 이식합니다!
	ACH6_Shop_SimulatorCharacter* PlayerChar = Cast<ACH6_Shop_SimulatorCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerChar)
	{
		MaxHealth = PlayerChar->FinalMaxHealth;
		CurrentHealth = MaxHealth; // 피통 꽉 채우고 디펜스 시작! ✅
	}

	// HUD가 시작 값을 표시할 수 있게 한 번 알림
	NotifyHealthChanged();
}

void ABaseCore::NotifyHealthChanged()
{
	OnCoreHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

ABaseCore* ABaseCore::GetBaseCore(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ABaseCore> It(World); It; ++It)
	{
		if (ABaseCore* Core = *It)
		{
			return Core;
		}
	}
	return nullptr;
}

float ABaseCore::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 체력이 깎였으니 HUD에 알림
	NotifyHealthChanged();

	// ★ 체력이 다 닳았으면 게임 오버. 중복 호출을 막아서 한 번만 처리함
	// (여러 몬스터가 같은 프레임에 때릴 수 있음).
	if (!bHasTriggeredGameOver && CurrentHealth <= 0)
	{
		bHasTriggeredGameOver = true;

		OnCoreDestroyed(); // 연출은 블루프린트에서
		AGameTimeManager::RequestGameOver(this);
	}

	return Applied;
}