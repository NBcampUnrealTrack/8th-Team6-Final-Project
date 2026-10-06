// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DefenseObjectBase.generated.h"

UCLASS()
class CH6_SHOP_SIMULATOR_API ADefenseObjectBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADefenseObjectBase();

	// 1. 현재 체력과 최대 체력 가방
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense | Health")
	int32 CurrentHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense | Health")
	int32 MaxHealth = 100;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 2. 몬스터에게 얻어맞았을 때 체력을 깎는 언리얼 표준 대미지 함수 오버라이드
	virtual float TakeDamage(float DamageAmount, 
		struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
};
