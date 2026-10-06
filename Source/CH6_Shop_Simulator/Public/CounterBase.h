// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CounterBase.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API ACounterBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACounterBase();

	// 1. 계산대 테이블 메쉬 (루트)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Counter")
	class UStaticMeshComponent* CounterMesh;

	// 2. 손님이 계산하러 와서 서 있을 발바닥 위치 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Counter")
	class USceneComponent* CustomerWaitSlot;

	// 3. 손님이 돈(지폐 액터)을 내려놓을 테이블 위 좌표 컴포넌트 - 이거는 필요없을듯 함. 08.03 기준 안쓰는중
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Counter")
	class USceneComponent* MoneySpawnSlot;

	// ★ [대기 줄 장부 👥]: 현재 내 계산대 뒤로 줄을 서 있는 손님들의 목록 배열!
	// 0번 인덱스에 있는 손님이 '현재 계산 중인 맨 앞 손님'이 됩니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Counter")
	TArray<class ACustomerAI*> CustomerQueue;

	// 손님이 줄 뒤에 서기 위해 계산대에게 자리를 요청하는 함수
	// 이 함수는 손님에게 "너는 몇 번째 칸에 가서 서 있어라" 하고 줄 번호(Index)를 알려줍니다.
	int32 JoinQueue(class ACustomerAI* NewCustomer);

	// 계산이 끝나서 맨 앞 손님이 나갔을 때, 뒷사람들을 한 칸씩 앞으로 당기는 함수
	void PopQueue();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
