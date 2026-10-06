// Fill out your copyright notice in the Description page of Project Settings.


#include "DefenseObjectBase.h"

// Sets default values
ADefenseObjectBase::ADefenseObjectBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ADefenseObjectBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADefenseObjectBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float ADefenseObjectBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 체력 차감 연산
	CurrentHealth = FMath::Clamp(CurrentHealth - FMath::FloorToInt(ActualDamage), 0, MaxHealth);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("[피격] [%s] 가 대미지를 입었습니다! 잔여 체력: %d / %d"), *GetName(), CurrentHealth, MaxHealth));

	// 만약 체력이 0이 되어 파괴되었다면?
	if (CurrentHealth <= 0)
	{
		// 이 자식 클래스들(기지나 성벽)이 각자 알아서 죽을 때 행동을 정의하게 만듭니다. (예: 게임오버 또는 단순 파괴)
		// 지금은 안전하게 액터만 월드에서 소멸시킵니다.
		Destroy();
	}

	return ActualDamage;
}