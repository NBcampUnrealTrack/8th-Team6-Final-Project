// Fill out your copyright notice in the Description page of Project Settings.
//
// 병사를 최대 3명 소환/유지하는 타워. 회전하거나 발사하지 않고, 그냥 주기적으로
// "지금 살아있는 병사가 3명 안 되면 빈 자리에 새로 소환"만 반복함.
// 실제 감지/이동/전투는 각 ASoldierUnit이 알아서 처리함.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoldierTower.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ASoldierUnit;
class AMonsterBase;

UCLASS()
class CH6_SHOP_SIMULATOR_API ASoldierTower : public AActor
{
	GENERATED_BODY()

public:
	ASoldierTower();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	// 병사가 스폰될 자리 3곳. 에디터에서 각각 원하는 위치로 옮겨서 길목에 나란히 세울 수 있음.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<USceneComponent>> SoldierSlots;

	UPROPERTY(EditDefaultsOnly, Category = "Soldier Tower")
	TSubclassOf<ASoldierUnit> SoldierClass;

	// 병사가 죽고 나서 이 시간(초)이 지나야 그 자리에 새로 부활함.
	// (예전엔 몇 초마다 전체 슬롯을 확인해서 즉시 채웠는데, 이제는 "죽은 그 순간"부터
	// 개별적으로 카운트다운을 시작해서 그 병사만 정해진 시간 후 부활함)
	UPROPERTY(EditAnywhere, Category = "Soldier Tower")
	float RespawnDelay = 20.f;

	// ★ 감지 판정의 중심점. 뷰포트에서 병사 슬롯들 가운데쯤으로 옮겨두면 됨.
	// 예전엔 "살아있는 병사들의 평균 위치"로 계산했는데, 그러면 병사가 죽을 때마다
	// 중심이 옮겨져서 그쪽 구역 감지가 약해지는 문제가 있었음. 고정 포인트로 두면
	// 병사가 몇 명 살아있든 이 타워가 지키는 구역이 항상 일정하게 유지됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> DetectionCenter;

	// 이 범위 안의 몬스터들을 병사들에게 배정 대상으로 삼음
	UPROPERTY(EditAnywhere, Category = "Soldier Tower")
	float GroupDetectionRange = 500.f;

	// ★ 감지 범위를 화면에 원으로 표시. 평소엔 꺼져있다가, 플레이어가 이 타워를
	// 바라보면(InteractableComponent의 하이라이트가 켜지면) 자동으로 같이 켜짐.
	// 에디터에서 직접 true로 켜두면 항상 표시됨 (밸런싱 작업용).
	UPROPERTY(EditAnywhere, Category = "Soldier Tower|Debug")
	bool bShowRangeDebug = false;

	// ★ 보스의 "전쟁의 함성" 등으로 일시적으로 병사 배정이 멈춤.
	// 소속 병사들도 함께 무력화됨.
	UFUNCTION(BlueprintCallable, Category = "Soldier Tower")
	void DisableForDuration(float Duration);

	UPROPERTY(BlueprintReadOnly, Category = "Soldier Tower")
	bool bIsDisabled = false;

	// 무력화 중 재생할 몽타주 (타워 본체에 스켈레탈 메시가 있는 경우)
	UPROPERTY(EditDefaultsOnly, Category = "Soldier Tower|Stun")
	TObjectPtr<class UAnimMontage> StunMontage;

	// 무력화 중 띄울 파티클
	UPROPERTY(EditDefaultsOnly, Category = "Soldier Tower|Stun")
	TObjectPtr<class UNiagaraSystem> StunEffect;

	// ★ 파티클이 나올 위치. 블루프린트 뷰포트에서 눈으로 보며 옮기면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> StunEffectPoint;

	virtual void Tick(float DeltaTime) override;

	// ★ 감지 판정의 실제 기준점 (DetectionCenter 컴포넌트 위치).
	// 디버그 원도 이 위치에 그려서 실제 판정 범위와 항상 일치시킴.
	FVector GetGroupCenter() const;

protected:
	// 플레이어가 이 타워를 바라볼 때/시선을 뗄 때 호출됨 (범위 표시 연동용)
	UFUNCTION()
	void HandleHighlightChanged(bool bHighlighted);

public:

	// ★ 타워 본체(슬롯 배치)는 그대로 두고, 스폰되는 각 병사"만" 이만큼 더 회전시켜서
	// 바라보는 방향을 보정함. 타워 전체를 돌리면 슬롯 좌우 배치까지 같이 뒤집혀버리는
	// 문제가 있어서, 위치(배치)와 방향(보정)을 분리해서 처리하기 위함.
	// (예: 서로 마주보는 두 타일에 같은 타워를 짓는데 한쪽만 뒤돌아 서있을 때 사용)
	UPROPERTY(EditAnywhere, Category = "Soldier Tower")
	float FacingOffsetYaw = 0.f;

	// 배정을 다시 확인하는 주기 (매 프레임 계산할 필요 없음)
	UPROPERTY(EditAnywhere, Category = "Soldier Tower | Performance")
	float AssignInterval = 0.3f;

private:
	// 특정 슬롯 하나에만 병사를 스폰 (초기 채우기, 개별 부활 둘 다 이 함수 재사용)
	void SpawnSoldierAtSlot(int32 SlotIndex);

	// 병사가 죽으면(Destroy) 이 함수가 호출됨. 어느 슬롯 소속이었는지 찾아서
	// 그 슬롯 하나에 대해서만 RespawnDelay 후 부활 타이머를 걺.
	UFUNCTION()
	void HandleSoldierDestroyed(AActor* DestroyedActor);

	void AssignSoldiers();

	UPROPERTY()
	TArray<TObjectPtr<ASoldierUnit>> SpawnedSoldiers;

	// 슬롯마다 개별 부활 타이머 (죽은 그 병사만 자기 타이머로 돌아감)
	TArray<FTimerHandle> RespawnTimerHandles;

	FTimerHandle AssignTimerHandle;

	// 무력화 해제용 타이머와 재생 중인 파티클
	FTimerHandle DisableTimerHandle;

	UPROPERTY()
	TObjectPtr<class UNiagaraComponent> ActiveStunEffect;

	void EndDisable();
};