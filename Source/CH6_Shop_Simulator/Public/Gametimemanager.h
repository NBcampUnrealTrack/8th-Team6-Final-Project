// Fill out your copyright notice in the Description page of Project Settings.
//
// 낮(경영)/밤(디펜스) 전환을 총괄하는 매니저.
// - 낮: 9시~18시, 실제 시간이 흐르면서 게임 내 시계도 같이 진행됨
// - 밤: 18시가 되면 자동으로 WaveManager::StartNight() 호출
// - 밤이 끝나는 시점은 "정해진 시간"이 아니라 "모든 웨이브 클리어"로 결정됨
//   (그래서 밤에는 숫자 시계 대신 "웨이브 진행 상황" 표시로 UI를 바꾸는 걸 추천)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameTimeManager.generated.h"

class AWaveManager;
class AMyCustomerSpawner;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Day,
	Night
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGameTimeUpdated, int32, Day, float, Hour, EGamePhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhaseChangedToDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhaseChangedToNight);

UCLASS()
class CH6_SHOP_SIMULATOR_API AGameTimeManager : public AActor
{
	GENERATED_BODY()

public:
	AGameTimeManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 웨이브 진행을 맡길 매니저 (레벨에서 지정)
	UPROPERTY(EditAnywhere, Category = "Game Time")
	TObjectPtr<AWaveManager> WaveManager;

	// 낮 시작 시각 / 밤 시작 시각 (24시간제 기준)
	UPROPERTY(EditAnywhere, Category = "Game Time")
	float DayStartHour = 9.f;

	UPROPERTY(EditAnywhere, Category = "Game Time")
	float NightStartHour = 18.f;

	// 낮(9시~18시, 즉 9시간)이 실제로는 몇 초 동안 흐르게 할지
	UPROPERTY(EditAnywhere, Category = "Game Time")
	float DayLengthInRealSeconds = 300.f; // 기본 5분

	// 날짜가 지날 때마다 웨이브 난이도에 곱해질 배율 증가폭 (1.15 = 하루 지날 때마다 15% 증가)
	UPROPERTY(EditAnywhere, Category = "Game Time")
	float DifficultyGrowthPerDay = 1.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Game Time")
	int32 CurrentDay = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Game Time")
	float CurrentHour;

	UPROPERTY(BlueprintReadOnly, Category = "Game Time")
	EGamePhase CurrentPhase = EGamePhase::Day;

	// 낮이 되긴 했지만, 플레이어가 오픈 간판을 누르기 전까지는 시간이 안 흐름
	UPROPERTY(BlueprintReadOnly, Category = "Game Time")
	bool bIsTimeRunning = false;

	// 손님 스포너 (오픈하면 켜고, 18시가 되면 자동으로 끔)
	UPROPERTY(EditAnywhere, Category = "Game Time")
	TObjectPtr<class AMyCustomerSpawner> CustomerSpawner;

	// 오픈 간판(AOpenSignActor 등)에서 플레이어가 상호작용하면 호출
	UFUNCTION(BlueprintCallable, Category = "Game Time")
	void OpenShop();

	// 18시가 되어 "밤" 상태로 전환된 후, 실제로 몬스터가 나오기까지의 준비 시간(초).
	// 이 동안 플레이어가 기차를 타러 가거나 마지막 배치를 할 수 있음.
	UPROPERTY(EditAnywhere, Category = "Game Time")
	float NightPrepDuration = 10.f;

	// 준비 시간이 시작될 때 UI에 알려주기 위한 델리게이트 (예: "밤이 옵니다..." 문구 표시용)
	UPROPERTY(BlueprintAssignable, Category = "Game Time")
	FOnPhaseChangedToNight OnNightPrepStarted;

	// UI가 구독해서 시계/날짜를 표시할 델리게이트 (낮 동안 매 프레임 갱신됨)
	UPROPERTY(BlueprintAssignable, Category = "Game Time")
	FOnGameTimeUpdated OnGameTimeUpdated;

	// 낮/밤으로 바뀌는 "순간"에 한 번씩만 알려주는 델리게이트 (UI를 시계 <-> 웨이브 표시로 전환할 때 사용)
	UPROPERTY(BlueprintAssignable, Category = "Game Time")
	FOnPhaseChangedToDay OnPhaseChangedToDay;

	UPROPERTY(BlueprintAssignable, Category = "Game Time")
	FOnPhaseChangedToNight OnPhaseChangedToNight;

	// 화면 좌상단에 띄울 시계 위젯 클래스 (UGameClockWidget을 부모로 만든 BP 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Game Time | UI")
	TSubclassOf<class UGameClockWidget> ClockWidgetClass;

	// ★ 코어가 파괴됐을 때 호출. 게임 오버 화면을 띄우고 게임을 멈춤.
	// 코어 블루프린트에서 체력이 0이 되는 시점에 이 함수를 부르면 됨.
	UFUNCTION(BlueprintCallable, Category = "Game Time")
	void TriggerGameOver();

	// ★ 레벨에서 GameTimeManager를 찾아 게임 오버를 발생시킴.
	// 코어 블루프린트처럼 매니저 참조가 없는 곳에서 편하게 쓰라고 만든 것.
	UFUNCTION(BlueprintCallable, Category = "Game Time", meta = (WorldContext = "WorldContext"))
	static void RequestGameOver(const UObject* WorldContext);

	// ★ HUD 안에 시계 위젯을 넣어 쓰는 경우, HUD가 이 함수로 자기 시계를
	// 등록해주면 됨. 그러면 매니저가 그 시계를 갱신함.
	// (매니저가 따로 시계를 만들지 않도록 ClockWidgetClass는 비워둘 것)
	UFUNCTION(BlueprintCallable, Category = "Game Time")
	void RegisterClockWidget(class UGameClockWidget* InClockWidget);

	// ★ 레벨의 GameTimeManager를 찾아서 반환. 위젯처럼 참조가 없는 곳에서
	// 매니저 함수를 부를 때 쓰면 됨.
	UFUNCTION(BlueprintCallable, Category = "Game Time", meta = (WorldContext = "WorldContext"))
	static AGameTimeManager* GetGameTimeManager(const UObject* WorldContext);

	// ===== 저장 / 불러오기 =====

	// ★ 현재 진행 상황을 저장. 하루가 시작될 때 자동으로 호출됨.
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveGameProgress();

	// ★ 저장된 데이터를 불러와 적용. 메인 메뉴에서 LOAD GAME으로 들어왔을 때
	// 게임 시작 시 한 번 호출됨.
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadGameProgress();

	// 저장 파일이 있는지 (메인 메뉴에서 LOAD GAME 버튼을 활성화할지 판단용)
	UFUNCTION(BlueprintCallable, Category = "Save", meta = (WorldContext = "WorldContext"))
	static bool HasSaveData(const UObject* WorldContext);

	// ★ 이 레벨을 이어하기로 시작할지 여부.
	// 메인 메뉴에서 LOAD GAME을 누르면 true로 설정한 뒤 레벨을 열면 됨.
	// (GameInstance에 두는 게 정석이지만, 간단하게 static으로 처리)
	static bool bShouldLoadOnStart;

	// 게임 오버 화면 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Game Time")
	TSubclassOf<class UGameOverWidget> GameOverWidgetClass;

	// 이미 게임이 끝났는지 (중복 호출 방지)
	UPROPERTY(BlueprintReadOnly, Category = "Game Time")
	bool bIsGameOver = false;

private:
	UPROPERTY()
	TObjectPtr<class UGameClockWidget> ClockWidgetInstance;

	void StartDay();
	void StartNight();
	void BeginNightWaves();

	FTimerHandle NightPrepTimerHandle;

	UFUNCTION()
	void HandleAllWavesComplete();

	// ★ 실제로 다음 웨이브가 시작되는 순간 호출됨 (인터미션 시작 시점이 아님).
	// 이때만 시계 UI의 웨이브 번호를 갱신하므로, 매 프레임 갱신할 필요가 없음.
	UFUNCTION()
	void HandleWaveChanged();

	// 시계 UI를 지금 상태로 갱신 (여러 곳에서 재사용)
	void RefreshClockWidget();
};