// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomerAI.h"
#include "MyCustomerSpawner.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API AMyCustomerSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyCustomerSpawner();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void StopSpawning();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 에디터에서 마우스로 다양하게 조립한 마네킹 블루프린트들을 담아둘 배열 👥
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner Config")
	TArray<TSubclassOf<class ACustomerAI>> CustomerBlueprintPool;

	// 6초마다 무한 작동할 타이머 전용 함수
	void SpawnLoop();
	FTimerHandle SpawnTimerHandle;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
