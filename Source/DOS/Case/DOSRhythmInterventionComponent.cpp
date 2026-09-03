#include "DOSRhythmInterventionComponent.h"

#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "DOSRhythmIntervention"

UDOSRhythmInterventionComponent::UDOSRhythmInterventionComponent()
{
	PromptText = LOCTEXT("KnockPrompt", "두드리기");

	// "똑  똑  똑똑똑  똑  똑똑"  - 타격 8번, 간격 7개.
	//   똑 [긴] 똑 [긴] 똑 [짧] 똑 [짧] 똑 [긴] 똑 [긴] 똑 [짧] 똑
	// 긴 0.45 / 짧 0.18 이면 비율이 2.5:1이라 일정한 연타로는 뚫리지 않는다
	// (연타 시 짧은 간격의 상대오차가 115%까지 벌어진다).
	PatternGapSeconds = { 0.45f, 0.45f, 0.18f, 0.18f, 0.45f, 0.45f, 0.18f };
}

EDOSInteractResult UDOSRhythmInterventionComponent::Interact()
{
	const UWorld* World = GetWorld();
	if (!World || PatternGapSeconds.Num() == 0)
	{
		return Super::Interact(); // 패턴이 없으면 평범한 밀기로 동작한다.
	}

	// 이미 이번 회차에 성립했으면 더 받지 않는다.
	if (IsActiveThisRound())
	{
		return EDOSInteractResult::Rejected;
	}

	// **실제 시간(배율 안 걸린 것)으로 잰다.** GetTimeSeconds()는 전역 시간 배율이 걸려
	// 있어서, ×0.5로 느리게 보는 중이면 같은 리듬을 쳐도 간격이 절반으로 기록된다.
	// 템포 배율 허용 범위(0.5~2.0)의 경계에 걸리고 ×0.25에서는 아예 실패한다.
	// 손이 두드리는 속도는 화면 배속과 무관하므로 실시간이 맞다.
	const double Now = World->GetRealTimeSeconds();

	if (TapTimes.Num() > 0)
	{
		const double SinceLast = Now - TapTimes.Last();
		if (SinceLast < MinTapIntervalSeconds)
		{
			return EDOSInteractResult::Acknowledged; // 키 반복. 삼킨다.
		}
		if (SinceLast > ResetTimeoutSeconds)
		{
			TapTimes.Reset(); // 손을 뗐다. 처음부터.
		}
	}

	TapTimes.Add(Now);

	// **슬라이딩 윈도우**: 필요한 개수만 남긴다. 시작을 헛쳐도 뒤에서부터 맞으면 성립하므로
	// 한 번 삐끗할 때마다 처음부터 다시 칠 필요가 없다.
	const int32 Needed = GetTapsNeeded();
	while (TapTimes.Num() > Needed)
	{
		TapTimes.RemoveAt(0, 1, EAllowShrinking::No);
	}

	OnKnock.Broadcast(TapTimes.Num(), Needed);

	if (TapTimes.Num() < Needed || !MatchesPattern())
	{
		return EDOSInteractResult::Acknowledged;
	}

	if (!TryIntervene())
	{
		// 리듬은 맞았는데 시간창 밖이다. 버퍼는 비워서 계속 두드리게 둔다.
		TapTimes.Reset();
		return EDOSInteractResult::Rejected;
	}

	TapTimes.Reset();
	return EDOSInteractResult::Succeeded;
}

bool UDOSRhythmInterventionComponent::MatchesPattern() const
{
	const int32 GapCount = PatternGapSeconds.Num();
	if (TapTimes.Num() != GapCount + 1)
	{
		return false;
	}

	// 최소제곱으로 템포 배율 s를 구한다:  s = Σ(g·t) / Σ(t·t)
	// 플레이어가 전체를 느리게/빠르게 쳐도 그 배율이 s로 흡수된다.
	double Numerator = 0.0;
	double Denominator = 0.0;
	for (int32 i = 0; i < GapCount; ++i)
	{
		const double Actual = TapTimes[i + 1] - TapTimes[i];
		const double Target = PatternGapSeconds[i];
		if (Target <= KINDA_SMALL_NUMBER)
		{
			return false; // 0초 간격은 판정할 수 없다.
		}
		Numerator += Actual * Target;
		Denominator += Target * Target;
	}
	if (Denominator <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const double Scale = Numerator / Denominator;
	if (Scale < MinTempoScale || Scale > MaxTempoScale)
	{
		return false; // 터무니없이 느리거나 빠르다.
	}

	// 그 배율 기준으로 간격마다 상대오차를 본다. 하나라도 넘으면 실패.
	for (int32 i = 0; i < GapCount; ++i)
	{
		const double Actual = TapTimes[i + 1] - TapTimes[i];
		const double Expected = Scale * PatternGapSeconds[i];
		if (Expected <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		if (FMath::Abs(Actual - Expected) / Expected > RelativeTolerance)
		{
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
