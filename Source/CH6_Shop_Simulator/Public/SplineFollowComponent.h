// SplineFollowComponent.h
// 몬스터와 기차(나중에 만들 ATrainPawn)가 공통으로 사용할 스플라인 추종 로직.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "SplineFollowComponent.generated.h"

class USplineComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CH6_SHOP_SIMULATOR_API USplineFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USplineFollowComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Follow")
	TObjectPtr<USplineComponent> TargetSpline = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Spline Follow")
	float DistanceAlongSpline = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Follow")
	float BaseSpeed = 300.f;

	// 둔화/버프 배율. 1.0 = 정상, 0.5 = 절반 속도, 0 = 정지
	UPROPERTY(BlueprintReadOnly, Category = "Spline Follow")
	float SpeedMultiplier = 1.f;

	// ★ 이동 가능한 최대 거리. 음수면 제한 없음(스플라인 끝까지).
	// 안개 장벽처럼 아직 못 가는 구간이 있을 때 기차가 그 앞까지만 가도록
	// 막는 용도. 기차는 위치를 직접 지정하는 방식이라 콜리전으로는 안 막히므로
	// 이렇게 거리로 제한해야 함.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Follow")
	float MaxAllowedDistance = -1.f;

	// 외부(몬스터/기차 Tick)에서 매 프레임 호출. DeltaDistance는 +/- 둘 다 가능 (기차 후진 대응)
	void AdvanceByDelta(float DeltaDistance);

	void SetDistance(float NewDistance);

	float GetProgressRatio() const;

	bool HasReachedEnd() const;

	// 가장 최근 이동(Sweep)에서 뭔가에 부딪혔는지. 몬스터가 "충돌한 순간 = 막혔다"를
	// 판정할 때 거리 계산 없이 이 값만 확인하면 됨.
	UPROPERTY(BlueprintReadOnly, Category = "Spline Follow")
	FHitResult LastMoveHit;

	// 스플라인 좌우 오프셋에서 얼마나 띄워서 액터를 위치시킬지.
	// 액터 원점 = 캡슐 중심이기 때문에, 캡슐 Half Height만큼 띄워줘야 바닥에 파묻히지 않음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Follow")
	float GroundOffsetZ = 88.f; // 몬스터 캡슐 Half Height(88)에 맞춰 기본값 설정. 다른 크기면 수정.

	// 스플라인 중심선 기준 좌우로 얼마나 떨어져서 걷는지. 몬스터마다 살짝씩 다르게 주면
	// 한 줄로 딱 붙어 오지 않고 자연스럽게 무리 지어 걷고, 막혔을 때도 부채꼴로 퍼져서 대기하게 됨.
	UPROPERTY(BlueprintReadOnly, Category = "Spline Follow")
	float LateralOffset = 0.f;

	// ★ 안전핀: 이동이 순수 텔레포트라서 콜리전으로 걸러지지 않으므로, LateralOffset이
	// 이 값을 절대 넘지 않도록 강제함. 통로 폭보다 좁게 잡아둬야 몬스터가 벽을 뚫고 나가지 않음.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Follow")
	float MaxLateralOffset = 80.f; // 레벨의 실제 통로 폭에 맞춰 조정할 것

	// LateralOffset을 설정할 때는 항상 이 함수를 통해서 설정 (MaxLateralOffset 범위로 강제 클램프됨)
	UFUNCTION(BlueprintCallable, Category = "Spline Follow")
	void SetLateralOffsetSafe(float DesiredOffset)
	{
		LateralOffset = FMath::Clamp(DesiredOffset, -MaxLateralOffset, MaxLateralOffset);
	}

private:
	void UpdateActorTransform();
};