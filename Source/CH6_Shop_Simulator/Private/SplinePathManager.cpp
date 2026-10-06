// Fill out your copyright notice in the Description page of Project Settings.

#include "SplinePathManager.h"
#include "DefenseWall.h"
#include "Components/SplineComponent.h"

void USplinePathManager::RegisterKillZone(USplineComponent* Spline, float Distance, ADefenseWall* Barricade)
{
	if (!Spline)
	{
		return;
	}

	TArray<FKillZoneEntry>& List = KillZonesBySpline.FindOrAdd(Spline);

	FKillZoneEntry NewEntry;
	NewEntry.Distance = Distance;
	NewEntry.Barricade = Barricade;
	List.Add(NewEntry);

	// 거리 순으로 정렬해둬야 몬스터가 "다음 킬존"을 순서대로 찾을 수 있음
	List.Sort([](const FKillZoneEntry& A, const FKillZoneEntry& B)
		{
			return A.Distance < B.Distance;
		});
}

bool USplinePathManager::GetKillZoneAtIndex(USplineComponent* Spline, int32 Index, float& OutDistance, ADefenseWall*& OutBarricade) const
{
	const TArray<FKillZoneEntry>* List = KillZonesBySpline.Find(Spline);
	if (!List || !List->IsValidIndex(Index))
	{
		return false; // 이 인덱스에 킬존이 없음 = 몬스터 입장에선 "더 이상 막힐 곳 없이 코어까지 직행"
	}

	OutDistance = (*List)[Index].Distance;
	OutBarricade = (*List)[Index].Barricade.Get();
	return true;
}