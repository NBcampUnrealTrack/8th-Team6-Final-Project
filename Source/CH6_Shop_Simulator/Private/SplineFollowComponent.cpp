// SplineFollowComponent.cpp

#include "SplineFollowComponent.h"
#include "Components/SplineComponent.h"

USplineFollowComponent::USplineFollowComponent()
{
	// 이동 갱신은 소유 액터(몬스터/기차)의 Tick에서 AdvanceByDelta를 호출하는 방식으로 제어.
	PrimaryComponentTick.bCanEverTick = false;
}

void USplineFollowComponent::AdvanceByDelta(float DeltaDistance)
{
	if (!TargetSpline)
	{
		return;
	}

	const float SplineLength = TargetSpline->GetSplineLength();

	// ★ 이동 상한이 설정돼 있으면 그 지점까지만 갈 수 있음 (안개 장벽 등).
	// 음수면 제한 없이 스플라인 끝까지.
	const float UpperLimit = (MaxAllowedDistance >= 0.f)
		? FMath::Min(MaxAllowedDistance, SplineLength) : SplineLength;

	DistanceAlongSpline = FMath::Clamp(DistanceAlongSpline + (DeltaDistance * SpeedMultiplier), 0.f, UpperLimit);

	UpdateActorTransform();
}

void USplineFollowComponent::SetDistance(float NewDistance)
{
	if (!TargetSpline)
	{
		return;
	}

	const float SplineLength = TargetSpline->GetSplineLength();
	DistanceAlongSpline = FMath::Clamp(NewDistance, 0.f, SplineLength);

	UpdateActorTransform();
}

float USplineFollowComponent::GetProgressRatio() const
{
	if (!TargetSpline || TargetSpline->GetSplineLength() <= 0.f)
	{
		return 0.f;
	}
	return DistanceAlongSpline / TargetSpline->GetSplineLength();
}

bool USplineFollowComponent::HasReachedEnd() const
{
	if (!TargetSpline)
	{
		return false;
	}
	return DistanceAlongSpline >= (TargetSpline->GetSplineLength() - 1.f);
}

void USplineFollowComponent::UpdateActorTransform()
{
	if (!TargetSpline || !GetOwner())
	{
		return;
	}

	const FVector RightVector = TargetSpline->GetRightVectorAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);

	const FVector NewLocation = TargetSpline->GetLocationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World)
		+ FVector(0.f, 0.f, GroundOffsetZ)
		+ (RightVector * LateralOffset);
	const FRotator NewRotation = TargetSpline->GetRotationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);

	// bSweep = true: 벽/바리케이드 같은 고정 지형에는 실제로 막힘.
	// 몬스터끼리는 AMonsterBase::BeginPlay에서 콜리전 응답을 Ignore로 강제 설정해두기 때문에
	// 서로는 무시하고 지나감. 길목이 좁으면 벽이 물리적으로 줄을 세우고, 넓으면 자동으로
	// 여러 마리가 나란히 갈 공간이 생기므로 폭이 다른 여러 길목에도 별도 설정 없이 대응됨.
	GetOwner()->SetActorLocationAndRotation(NewLocation, NewRotation, /*bSweep=*/true, &LastMoveHit);

	// ★ 막혀서 실제 위치가 계산값(DistanceAlongSpline)만큼 못 갔는데 그걸 그냥 두면,
	// 막혀있는 동안 계속 커지기만 하다가 나중에 풀리는 순간 그 차이만큼 한번에
	// 앞으로 튀는(순간이동) 현상이 생김. 그렇다고 매번 되돌리면 살짝 스치기만 해도
	// 매 프레임 끊기는 것처럼 보임. 그래서 "차이가 작으면 무시, 크게 벌어지면 보정"으로 절충.
	if (LastMoveHit.bBlockingHit)
	{
		const FVector ActualLocation = GetOwner()->GetActorLocation();
		const float ActualKey = TargetSpline->FindInputKeyClosestToWorldLocation(ActualLocation);
		const float ActualDistance = TargetSpline->GetDistanceAlongSplineAtSplineInputKey(ActualKey);

		constexpr float DriftTolerance = 20.f; // 이 정도 차이는 무시 (끊김 방지용 여유)
		if (FMath::Abs(DistanceAlongSpline - ActualDistance) > DriftTolerance)
		{
			DistanceAlongSpline = ActualDistance;
		}
	}
}