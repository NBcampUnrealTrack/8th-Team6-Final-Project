// Fill out your copyright notice in the Description page of Project Settings.

#include "GameTimeManager.h"
#include "WaveManager.h"
#include "GameClockWidget.h"
#include "GameOverWidget.h"
#include "MyCustomerSpawner.h"
#include "ShopSaveGame.h"
#include "BuildTile.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "MaterialInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AGameTimeManager::AGameTimeManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGameTimeManager::BeginPlay()
{
	Super::BeginPlay();

	if (WaveManager)
	{
		// 밤이 끝나는 시점(모든 웨이브 클리어)을 구독해서 자동으로 다음 날 낮으로 전환
		WaveManager->OnAllWavesComplete.AddDynamic(this, &AGameTimeManager::HandleAllWavesComplete);
		WaveManager->OnWaveStarted.AddDynamic(this, &AGameTimeManager::HandleWaveChanged);
	}

	if (ClockWidgetClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			ClockWidgetInstance = CreateWidget<UGameClockWidget>(PC, ClockWidgetClass);
			if (ClockWidgetInstance)
			{
				ClockWidgetInstance->AddToViewport();
			}
		}
	}

	// ★ 메인 메뉴에서 LOAD GAME으로 들어온 경우 저장 데이터를 먼저 적용.
	// StartDay()보다 앞에 있어야 불러온 날짜로 하루가 시작됨.
	if (bShouldLoadOnStart)
	{
		bShouldLoadOnStart = false; // 한 번만 적용 (다음 레벨 로드에 영향 없게)
		LoadGameProgress();
	}

	StartDay();
}

void AGameTimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 밤이거나, 아직 오픈 전(bIsTimeRunning == false)이면 시간이 안 흐름.
	// (밤의 웨이브 진행 표시는 매 프레임이 아니라, WaveManager가 웨이브 전환
	// 신호를 보낼 때만 갱신함 - HandleWaveChanged 참고)
	if (CurrentPhase != EGamePhase::Day || !bIsTimeRunning)
	{
		return;
	}

	// 하루(DayStartHour ~ NightStartHour) 분량의 게임 내 시간을,
	// DayLengthInRealSeconds 만큼의 실제 시간에 걸쳐 흐르게 함.
	const float GameHoursPerDay = NightStartHour - DayStartHour;
	const float HoursPerRealSecond = GameHoursPerDay / DayLengthInRealSeconds;

	CurrentHour += HoursPerRealSecond * DeltaTime;
	OnGameTimeUpdated.Broadcast(CurrentDay, CurrentHour, CurrentPhase);
	RefreshClockWidget();

	if (CurrentHour >= NightStartHour)
	{
		StartNight();
	}
}

void AGameTimeManager::OpenShop()
{
	// 낮인데 아직 시간이 안 흐르고 있을 때만 의미 있음 (밤이거나 이미 열려있으면 무시)
	if (CurrentPhase != EGamePhase::Day || bIsTimeRunning)
	{
		return;
	}

	bIsTimeRunning = true;

	if (CustomerSpawner)
	{
		CustomerSpawner->StartSpawning();
	}
}

void AGameTimeManager::TriggerGameOver()
{
	// 중복 호출 방지 (코어 파괴가 여러 번 통지될 수 있음)
	if (bIsGameOver)
	{
		return;
	}

	bIsGameOver = true;
	bIsTimeRunning = false;

	// 진행 중인 타이머 정리
	GetWorldTimerManager().ClearTimer(NightPrepTimerHandle);

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (GameOverWidgetClass && PC)
	{
		if (UGameOverWidget* GameOverWidget = CreateWidget<UGameOverWidget>(PC, GameOverWidgetClass))
		{
			GameOverWidget->AddToViewport();
			GameOverWidget->ShowResult(CurrentDay);
		}
	}

	if (PC)
	{
		// 버튼을 누를 수 있게 마우스 커서를 켜고 UI 입력 모드로
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	// ★ 게임을 멈춤. 위젯은 계속 동작해야 하므로 SetGamePaused를 씀
	// (액터 Tick은 멈추지만 UI 입력은 살아있음).
	UGameplayStatics::SetGamePaused(this, true);
}

bool AGameTimeManager::bShouldLoadOnStart = false;

AGameTimeManager* AGameTimeManager::GetGameTimeManager(const UObject* WorldContext)
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

	for (TActorIterator<AGameTimeManager> It(World); It; ++It)
	{
		if (AGameTimeManager* Manager = *It)
		{
			return Manager;
		}
	}
	return nullptr;
}

void AGameTimeManager::RegisterClockWidget(UGameClockWidget* InClockWidget)
{
	if (!InClockWidget)
	{
		return;
	}

	ClockWidgetInstance = InClockWidget;

	// 등록되자마자 현재 상태를 한 번 반영 (빈 화면으로 시작하지 않게)
	RefreshClockWidget();
}

void AGameTimeManager::SaveGameProgress()
{
	UShopSaveGame* SaveData = Cast<UShopSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UShopSaveGame::StaticClass()));

	if (!SaveData)
	{
		return;
	}

	SaveData->SavedDay = CurrentDay;

	// 플레이어 골드와 스탯
	if (ACH6_Shop_SimulatorCharacter* Player =
		Cast<ACH6_Shop_SimulatorCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		SaveData->SavedGold = Player->PlayerGold;

		SaveData->SavedBaseAttackPower = Player->BaseAttackPower;
		SaveData->SavedAttackMultiplier = Player->AttackMultiplier;
		SaveData->SavedBaseMaxHealth = Player->BaseMaxHealth;
		SaveData->SavedHealthMultiplier = Player->HealthMultiplier;

		// 보유 재료
		if (Player->MaterialInventory)
		{
			SaveData->SavedMaterials = Player->MaterialInventory->GetAllMaterials();
		}
	}

	// ★ 지어둔 구조물들. 타일 이름과 구조물 클래스만 저장해두면
	// 로드할 때 같은 타일에 같은 걸 다시 지어서 복원할 수 있음.
	for (TActorIterator<ABuildTile> It(GetWorld()); It; ++It)
	{
		ABuildTile* Tile = *It;
		if (!Tile || !Tile->bIsOccupied || !Tile->BuiltStructure)
		{
			continue;
		}

		FSavedStructure Entry;
		Entry.TileName = Tile->GetFName();
		Entry.StructureClassPath = FSoftClassPath(Tile->BuiltStructure->GetClass());
		Entry.PaidCost = Tile->PaidCost;

		SaveData->SavedStructures.Add(Entry);
	}

	UGameplayStatics::SaveGameToSlot(SaveData, UShopSaveGame::SlotName, 0);
}

bool AGameTimeManager::LoadGameProgress()
{
	UShopSaveGame* SaveData = Cast<UShopSaveGame>(
		UGameplayStatics::LoadGameFromSlot(UShopSaveGame::SlotName, 0));

	if (!SaveData)
	{
		return false;
	}

	CurrentDay = SaveData->SavedDay;

	if (ACH6_Shop_SimulatorCharacter* Player =
		Cast<ACH6_Shop_SimulatorCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		Player->PlayerGold = SaveData->SavedGold;

		Player->BaseAttackPower = SaveData->SavedBaseAttackPower;
		Player->AttackMultiplier = SaveData->SavedAttackMultiplier;
		Player->BaseMaxHealth = SaveData->SavedBaseMaxHealth;
		Player->HealthMultiplier = SaveData->SavedHealthMultiplier;

		// ★ 보유 재료 복원. 저장된 게 없으면(예전 세이브 파일) 건드리지 않아서
		// 시작 재료가 날아가지 않게 함.
		if (Player->MaterialInventory && SaveData->SavedMaterials.Num() > 0)
		{
			Player->MaterialInventory->SetAllMaterials(SaveData->SavedMaterials);
		}

		// ★ 원본 값을 넣었으니 최종값을 다시 계산하고 기차/코어에 반영.
		// 이걸 빼먹으면 스탯은 복원됐는데 실제로는 적용이 안 됨.
		Player->UpdateFinalStats();
	}

	// ★ 저장된 구조물을 원래 타일에 다시 지어줌.
	// 타일 이름으로 찾아서 매칭하므로, 레벨에서 타일을 지우거나 이름이
	// 바뀌면 그 타일 것은 복원되지 않음 (그냥 비어있는 상태로 시작).
	for (const FSavedStructure& Entry : SaveData->SavedStructures)
	{
		UClass* StructureClass = Entry.StructureClassPath.TryLoadClass<AActor>();
		if (!StructureClass)
		{
			continue;
		}

		for (TActorIterator<ABuildTile> It(GetWorld()); It; ++It)
		{
			ABuildTile* Tile = *It;
			if (Tile && Tile->GetFName() == Entry.TileName)
			{
				Tile->RestoreStructure(StructureClass, Entry.PaidCost);
				break;
			}
		}
	}

	return true;
}

bool AGameTimeManager::HasSaveData(const UObject* WorldContext)
{
	return UGameplayStatics::DoesSaveGameExist(UShopSaveGame::SlotName, 0);
}

void AGameTimeManager::RequestGameOver(const UObject* WorldContext)
{
	if (AGameTimeManager* Manager = GetGameTimeManager(WorldContext))
	{
		Manager->TriggerGameOver();
	}
}

void AGameTimeManager::StartDay()
{
	CurrentPhase = EGamePhase::Day;
	CurrentHour = DayStartHour;
	bIsTimeRunning = false; // 오픈 간판을 누르기 전까지는 대기

	// ★ 하루가 시작될 때마다 자동 저장. 밤을 넘긴 시점이라
	// "여기까지 성공했다"는 구간이 딱 맞음.
	SaveGameProgress();

	OnPhaseChangedToDay.Broadcast();
	OnGameTimeUpdated.Broadcast(CurrentDay, CurrentHour, CurrentPhase);
	RefreshClockWidget();
}

void AGameTimeManager::StartNight()
{
	CurrentPhase = EGamePhase::Night;
	CurrentHour = NightStartHour;
	bIsTimeRunning = false;

	if (CustomerSpawner)
	{
		CustomerSpawner->StopSpawning();
	}

	OnPhaseChangedToNight.Broadcast();
	OnNightPrepStarted.Broadcast();
	OnGameTimeUpdated.Broadcast(CurrentDay, CurrentHour, CurrentPhase);
	RefreshClockWidget();

	// ★ 18시가 됐다고 바로 몬스터가 쏟아지지 않도록, 준비 시간만큼 기다렸다가
	// 실제 웨이브를 시작함. 이 동안 플레이어가 기차 타러 이동하거나 마지막 배치 가능.
	GetWorldTimerManager().SetTimer(NightPrepTimerHandle, this, &AGameTimeManager::BeginNightWaves, NightPrepDuration, false);
}

void AGameTimeManager::BeginNightWaves()
{
	if (WaveManager)
	{
		// 날짜가 지날수록 세지도록 배율 갱신 (1일차=1.0, 2일차=1.15, 3일차=1.32...)
		const float Multiplier = FMath::Pow(DifficultyGrowthPerDay, static_cast<float>(CurrentDay - 1));
		WaveManager->SetDayMultiplier(Multiplier);
		WaveManager->StartNight(CurrentDay);

		// ★ StartNight에서 웨이브 인덱스가 0으로 초기화되므로, 그 직후 UI를 갱신해서
		// "1/3"부터 시작하게 함. (안 하면 지난 밤 마지막 값인 "4/3"이 그대로 남아있음)
		RefreshClockWidget();
	}
}

void AGameTimeManager::HandleAllWavesComplete()
{
	// 밤이 끝났음 = 다음 날로 넘어감
	CurrentDay++;
	StartDay();
}

void AGameTimeManager::HandleWaveChanged()
{
	// ★ 다음 웨이브로 넘어가는 순간에만 시계 UI를 갱신. 매 프레임 갱신하는
	// 것보다 훨씬 효율적이고, 어차피 웨이브 번호는 이때만 바뀜.
	RefreshClockWidget();
}

void AGameTimeManager::RefreshClockWidget()
{
	if (ClockWidgetInstance)
	{
		ClockWidgetInstance->UpdateDisplay(CurrentDay, CurrentHour, CurrentPhase,
			WaveManager ? WaveManager->GetCurrentWaveNumber() : 0,
			WaveManager ? WaveManager->GetTotalWaveCount() : 0);
	}
}