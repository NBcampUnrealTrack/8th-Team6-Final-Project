// Fill out your copyright notice in the Description page of Project Settings.


#include "ProductBase.h"

// Sets default values
AProductBase::AProductBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AProductBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 게임 시작(혹은 스폰) 시 데이터 테이블 가격표를 읽어옵니다.
	InitializeProductInfo();
}

// Called every frame
void AProductBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProductBase::InitializeProductInfo()
{
	if (ProductDataTable && !ProductRowName.IsNone())
	{
		// 데이터 테이블에서 고유 ID(RowName)에 해당하는 한 줄을 쏙 꺼내옵니다! 🔎
		FProductData* FoundData = ProductDataTable->FindRow<FProductData>(ProductRowName, TEXT(""));
		if (FoundData)
		{
			// 데이터 테이블에 적혀있는 기본 가격과 이름을 내 변수에 복사!
			ItemName = FoundData->ItemName;
			Price = FoundData->BasePrice; // 기본 단가 장착 💸

			// 🔥 [추가]: 엑셀에 적어둔 보너스 타입과 수치 값을 내 상자 변수에 복사!
			MyBonusType = FoundData->BonusType;
			MyBonusValue = FoundData->BonusValue;
		}
	}
}

