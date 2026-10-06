// Fill out your copyright notice in the Description page of Project Settings.
//
// 킹덤러시 스타일의 타격 텍스트. 몬스터가 맞거나 죽을 때 "POW!", "OOF!" 같은
// 만화풍 이미지가 잠깐 떠올랐다가 사라짐.
//
// 필요할 때만 스폰되는 액터라서, 몬스터가 죽어도 텍스트는 남아 끝까지 보이고
// 여러 개가 동시에 떠도 서로 간섭하지 않음.
//
// 사용법:
// 1. 이 클래스를 상속한 블루프린트를 만들고, 스프라이트(또는 위젯)에
//    "POW!" 같은 이미지를 지정
// 2. 각 타워/병사 블루프린트의 HitTextClass에 그 블루프린트를 넣어둠

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HitTextActor.generated.h"

class UPaperSpriteComponent;

UCLASS()
class CH6_SHOP_SIMULATOR_API AHitTextActor : public AActor
{
	GENERATED_BODY()

public:
	AHitTextActor();

	virtual void Tick(float DeltaTime) override;

	// 텍스트가 붙을 기준점. 이 아래에 스프라이트나 위젯을 블루프린트에서 추가하면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> TextRoot;

	// 떠있는 시간(초). 이 시간이 지나면 사라짐.
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	float LifeTime = 0.8f;

	// 위로 떠오르는 속도 (초당 유닛)
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	float FloatSpeed = 120.f;

	// ★ 스폰 위치에 줄 랜덤 흔들림 범위. 같은 자리에서 여러 개가 떠도
	// 겹쳐 보이지 않게 조금씩 흩어뜨림.
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	FVector RandomOffsetRange = FVector(40.f, 40.f, 20.f);

	// 기울기에 줄 랜덤 범위(도). 만화적인 느낌을 위해 살짝 삐뚤게.
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	float RandomRollRange = 15.f;

	// 등장할 때 튀어오르는 크기 배율 (1.2면 20% 커졌다가 원래대로)
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	float PopScale = 1.3f;

	// 튀어오르는 연출이 끝나는 시점 (전체 수명 대비 비율)
	UPROPERTY(EditDefaultsOnly, Category = "Hit Text")
	float PopDuration = 0.15f;

	// ★ 사라지기 직전 서서히 투명해지는 연출용. 블루프린트에서 이 값을 받아
	// 머티리얼 불투명도나 위젯 알파에 반영하면 됨 (1 = 완전 불투명, 0 = 투명).
	UFUNCTION(BlueprintImplementableEvent, Category = "Hit Text")
	void OnAlphaUpdated(float Alpha);

	// 스폰 직후 호출. 위치에 랜덤을 주고 수명 타이머를 시작함.
	UFUNCTION(BlueprintCallable, Category = "Hit Text")
	void InitHitText();

	// ★ 어디서든 편하게 띄울 수 있는 공용 함수.
	// TextClass가 없으면 아무 일도 안 일어나므로 안전하게 호출 가능.
	UFUNCTION(BlueprintCallable, Category = "Hit Text", meta = (WorldContext = "WorldContext"))
	static void SpawnHitText(const UObject* WorldContext, TSubclassOf<AHitTextActor> TextClass,
		const FVector& Location);

private:
	float ElapsedTime = 0.f;
	FVector BaseScale = FVector::OneVector;
};