// Fill out your copyright notice in the Description page of Project Settings.
//
// 아직 열리지 않은 구역을 막는 안개 장막. 지정한 날짜가 되면 길막이가 사라지고
// 안개가 서서히 옅어지면서 그 방향이 열림.
//
// 배치 방법:
// 1. 레벨에서 막고 싶은 길목에 이 액터를 배치
// 2. BlockMesh(자동 생성된 벽)의 크기를 길 폭에 맞게 조정
// 3. 블루프린트에서 안개 이펙트나 반투명 메시를 추가로 붙이면 됨
// 4. OpenOnDay에 열릴 날짜를 입력 (예: 11)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FogBarrier.generated.h"

class UBoxComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API AFogBarrier : public AActor
{
	GENERATED_BODY()

public:
	AFogBarrier();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 길을 물리적으로 막는 콜리전. 뷰포트에서 길 폭에 맞게 크기를 조정하면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> BlockVolume;

	// 안개 메시나 이펙트를 붙일 기준점. 블루프린트에서 이 아래에 추가하면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> FogRoot;

	// ★ 이 날짜가 되면 장벽이 열림 (예: 11이면 11일차부터 통행 가능)
	UPROPERTY(EditInstanceOnly, Category = "Fog Barrier")
	int32 OpenOnDay = 11;

	// 안개가 완전히 걷히는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Fog Barrier")
	float FadeOutDuration = 3.f;

	// ★ 기차는 스플라인 위 거리로 움직여서 콜리전으로는 안 막힘.
	// 이 장벽이 막고 있는 동안 기차가 갈 수 있는 최대 거리를 여기에 지정하면,
	// 장벽이 열릴 때 자동으로 해제됨. 음수면 기차는 제한하지 않음.
	// (레벨에서 장벽 위치에 해당하는 스플라인 거리를 입력할 것)
	UPROPERTY(EditInstanceOnly, Category = "Fog Barrier")
	float TrainMaxDistanceWhileClosed = -1.f;

	// 지금 열려있는지 (블루프린트에서 상태 확인용)
	UPROPERTY(BlueprintReadOnly, Category = "Fog Barrier")
	bool bIsOpen = false;

	// ★ 안개 농도. 1이면 완전히 짙고 0이면 투명.
	// 블루프린트에서 이 값을 받아 머티리얼 불투명도나 이펙트 세기에 반영하면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Fog Barrier")
	void OnFogDensityChanged(float Density);

	// 장벽이 열리기 시작하는 순간 발생 (사운드나 연출용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Fog Barrier")
	void OnBarrierOpened();

	// 현재 날짜를 받아서 열지 말지 판단. GameTimeManager가 날이 바뀔 때마다 호출함.
	UFUNCTION(BlueprintCallable, Category = "Fog Barrier")
	void UpdateForDay(int32 CurrentDay);

private:
	// 열리는 중인지 (페이드 진행 상태)
	bool bIsFading = false;
	float FadeElapsed = 0.f;
};