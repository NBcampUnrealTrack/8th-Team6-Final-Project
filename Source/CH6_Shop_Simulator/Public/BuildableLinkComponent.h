// Fill out your copyright notice in the Description page of Project Settings.
//
// 어떤 종류의 구조물(타워, 병사타워, 바리케이드 등)이든 상관없이, 스폰될 때
// 이 컴포넌트를 하나 붙여두면 "나는 이 BuildTile 소속이야"를 알 수 있게 해줌.
// 캐릭터가 지어진 구조물 자체를 클릭했을 때도, 그 구조물이 서있는 타일의
// 관리 메뉴(판매/업그레이드)를 띄울 수 있게 하기 위함 - 구조물 클래스마다 매번
// "나는 어느 타일 소속이다"를 따로 상속/구현할 필요 없이, 이 컴포넌트 하나로 해결.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuildableLinkComponent.generated.h"

class ABuildTile;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CH6_SHOP_SIMULATOR_API UBuildableLinkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Build")
	TObjectPtr<ABuildTile> OwningTile;
};