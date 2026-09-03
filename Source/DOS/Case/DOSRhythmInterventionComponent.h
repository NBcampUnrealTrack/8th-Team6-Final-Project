// 리듬 노크. 정해진 리듬으로 두드려야 성립하는 개입.
//
// 감옥(로즈의 은신처)은 보일러실 쪽에서 벽으로 보인다. 앨리스와 로즈가 오래 써온 암구호를
// 그대로 두드려야만 열린다 (Story_Design.md "로즈의 방 문 — 리듬 노크").
//
// **절대 시각이 아니라 간격의 비율로 판정한다.** 플레이어는 메트로놈이 아니라 눈으로 본
// 리듬을 흉내내는 것이라, 조금 느리게 두드렸다고 실패하면 부당하다. 최소제곱으로 템포
// 배율을 먼저 구하고 그 배율에 대한 상대오차를 보므로, 전체를 1.4배 느리게 쳐도 통과한다.
//
// ⚠ **간격이 전부 같은 패턴은 쓰지 말 것.** 비율로 보기 때문에 아무렇게나 일정하게 연타하면
// 통과해버린다. 긴 간격과 짧은 간격의 대비가 최소한 하나는 있어야 한다.

#pragma once

#include "CoreMinimal.h"
#include "DOSInterventionComponent.h"
#include "DOSRhythmInterventionComponent.generated.h"

// 두드릴 때마다 쏜다. **두드리는 소리를 여기에 걸지 않으면 플레이가 불가능하다** -
// 자기가 무슨 리듬을 치고 있는지 들리지 않으면 맞출 수가 없다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDOSOnKnock, int32, TapIndex, int32, TapsNeeded);

UCLASS(ClassGroup = (DOS), meta = (BlueprintSpawnableComponent))
class DOS_API UDOSRhythmInterventionComponent : public UDOSInterventionComponent
{
	GENERATED_BODY()

public:
	UDOSRhythmInterventionComponent();

	// 타격 **사이**의 간격(초). 항목이 7개면 두드리는 횟수는 8번이다.
	// 기본값은 "똑 똑 똑똑똑 똑 똑똑" - 긴 0.45 / 짧은 0.18 (비율 2.5:1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm")
	TArray<float> PatternGapSeconds;

	// 간격마다 허용하는 **상대**오차. 0.35면 35%다. 절대 밀리초가 아니라 비율이라
	// 느린 리듬에서는 자동으로 관대해지고 빠른 리듬에서는 자동으로 촘촘해진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float RelativeTolerance = 0.35f;

	// 허용하는 템포 배율 범위. 이걸 안 묶으면 터무니없이 느리게 쳐도 비율만 맞으면 통과한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm", meta = (ClampMin = "0.1"))
	float MinTempoScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm", meta = (ClampMin = "0.1"))
	float MaxTempoScale = 2.0f;

	// 마지막 타격에서 이만큼 지나면 처음부터 다시. 손을 뗀 것으로 본다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm", meta = (ClampMin = "0.2"))
	float ResetTimeoutSeconds = 1.6f;

	// 이보다 촘촘한 입력은 무시한다. 키 반복이나 더블 클릭으로 두 번 들어오는 것 방지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Rhythm", meta = (ClampMin = "0.0"))
	float MinTapIntervalSeconds = 0.06f;

	UPROPERTY(BlueprintAssignable, Category = "DOS|Rhythm")
	FDOSOnKnock OnKnock;

	virtual EDOSInteractResult Interact() override;

	// 지금까지 몇 번 두드렸는가. 디버그 표시용.
	UFUNCTION(BlueprintPure, Category = "DOS|Rhythm")
	int32 GetTapCount() const { return TapTimes.Num(); }

	// 패턴을 채우려면 몇 번 두드려야 하는가.
	UFUNCTION(BlueprintPure, Category = "DOS|Rhythm")
	int32 GetTapsNeeded() const { return PatternGapSeconds.Num() + 1; }

private:
	// 최근 타격 시각(월드 실시간). 타임라인 배속과 무관해야 하므로 게임 시각을 쓴다.
	TArray<double> TapTimes;

	// 지금 버퍼가 패턴과 맞는가. 템포 배율을 최소제곱으로 맞춘 뒤 상대오차를 본다.
	bool MatchesPattern() const;
};
