// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCustomerSpawner.h"
#include "CustomerZone.h"

// Sets default values
AMyCustomerSpawner::AMyCustomerSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMyCustomerSpawner::BeginPlay()
{
	Super::BeginPlay();

	//GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMyCustomerSpawner::SpawnLoop, 6.0f, true);
}
// Called every frame
void AMyCustomerSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void AMyCustomerSpawner::SpawnLoop()
{
	// 에디터 배열에 손님 종류가 하나도 등록 안 되어 있다면 탈출
	if (CustomerBlueprintPool.Num() == 0) return;

	// 2. ★ [랜덤 외형 픽스]: 배열에 들어있는 개수 중에서 무작위 순번(Index) 하나를 뽑습니다.
	int32 RandomIndex = FMath::RandRange(0, CustomerBlueprintPool.Num() - 1);
	TSubclassOf<ACustomerAI> ChosenClass = CustomerBlueprintPool[RandomIndex];

	if (!ChosenClass) return;

	// ★ 레벨에 배치한 Spawn 타입 CustomerZone 안에서 랜덤한 위치를 뽑음.
	// 손님마다 조금씩 다른 자리에서 나타나 자연스럽고, 상점을 옮겨도 마커만
	// 같이 옮기면 되므로 좌표를 코드에 박아둘 필요가 없음.
	// 마커가 없으면 예전처럼 이 스포너 액터 자리에서 스폰.
	FVector SpawnLoc = GetActorLocation();
	FRotator SpawnRot = GetActorRotation();

	if (ACustomerZone* SpawnZone = ACustomerZone::FindZone(this, ECustomerZoneType::Spawn))
	{
		SpawnLoc = SpawnZone->GetRandomPoint();
		SpawnRot = SpawnZone->GetActorRotation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 월드에 랜덤한 외형의 손님 스폰! 👥
	ACustomerAI* NewCustomer = GetWorld()->SpawnActor<ACustomerAI>(ChosenClass, SpawnLoc, SpawnRot, SpawnParams);
	if (NewCustomer)
	{
		NewCustomer->SpawnDefaultController(); // 뇌 수임
	}
}

void AMyCustomerSpawner::StartSpawning()
{
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMyCustomerSpawner::SpawnLoop, 6.0f, true);
}

void AMyCustomerSpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
}