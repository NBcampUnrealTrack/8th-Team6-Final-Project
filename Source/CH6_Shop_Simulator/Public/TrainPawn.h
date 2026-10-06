// Fill out your copyright notice in the Description page of Project Settings.
//
// 플레이어가 조종하는 기차. 몬스터와 같은 USplineFollowComponent를 재사용하되,
// 자동 증가 대신 입력값(전후진)으로 거리를 증감시킴.
// 카메라는 기차 몸체 회전과 분리(CameraPivot의 Absolute Rotation)해서, 코너를 돌 때
// 시야까지 같이 꺾이지 않게 함 (예전에 정해둔 원칙 - 안 그러면 멀미 유발).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TrainPawn.generated.h"

class UStaticMeshComponent;
class USplineFollowComponent;
class USceneComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ARailSwitchPoint;
struct FInputActionValue;

UCLASS()
class CH6_SHOP_SIMULATOR_API ATrainPawn : public APawn
{
	GENERATED_BODY()

public:
	ATrainPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ===== 컴포넌트 =====

	// ★ 액터의 실제 루트. 예전엔 생성자 안 지역 변수로 만들었는데, 그러면
	// 언리얼이 이 컴포넌트를 제대로 관리하지 못해서 에디터 디테일 창에도 안 뜨고
	// Mobility 같은 설정도 바꿀 수 없는 문제가 있었음 (ADisplayBase에서 겪은 것과 동일).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> TrainRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TrainMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USplineFollowComponent> SplineFollowComp;

	// 플레이어가 마우스로 조준하는 터렛 머리. Absolute Rotation을 켜서 기차 몸체
	// 회전과 완전히 분리시킴 - 코너를 돌아도 조준 방향이 안 틀어짐.
	// ★ 코드가 매 프레임 이 컴포넌트의 회전을 조준 방향으로 갱신하므로, 여기에
	// 직접 메시를 넣으면 수동 회전 보정을 할 수가 없음 (매번 덮어써짐).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretHeadComp;

	// ★ 실제로 눈에 보이는 터렛 메시. TurretHeadComp의 자식이라 조준 회전은
	// 그대로 따라가지만, 이 컴포넌트 자신의 Relative Rotation은 코드가 안 건드림.
	// 모델링 파일의 "정면" 축이 언리얼 기준(+X)과 다르게 만들어져서 Pitch를
	// 넣었을 때 옆으로 기우뚱한다면, 여기 Relative Rotation을 조정해서 보정하면 됨
	// (보통 Yaw 90/-90/180 중 하나로 해결됨).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TurretVisualMesh;

	// 카메라는 터렛 머리에 붙여서, 조준 방향이 곧 플레이어 시야가 되게 함
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// 탄환이 튀어나올 위치 (터렛 머리 앞쪽)
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	// 나중에 화물칸(ATrainCarWithTurret 등)을 연결할 지점
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> RearCouplingPoint;

	// ★ 하차할 때 플레이어 캐릭터가 서게 될 위치 (터렛 바로 뒤).
	// 탑승 중에 캐릭터가 갑판에서 미끄러지거나, 멀리서 클릭해서 탄 경우에도
	// 하차하면 항상 이 자리로 옮겨져서 엉뚱한 곳에 내리는 문제가 없어짐.
	// 뷰포트에서 터렛 뒤쪽 갑판 위 적당한 자리로 옮겨두면 됨.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> DismountPoint;

	// ===== 이동 설정 =====

	// 레일 스플라인을 가진 액터 (MonsterSpawner의 SplinePathActor와 같은 패턴)
	UPROPERTY(EditAnywhere, Category = "Train")
	AActor* RailSplineActor;

	// 기차가 스폰될 때 스플라인의 몇 번째 지점(인풋 키)에서 시작할지.
	// 조절점 번호와 대응됨 (0 = 첫 조절점, 1 = 두 번째 조절점, 2.5 = 2번과 3번 사이 중간 등)
	UPROPERTY(EditAnywhere, Category = "Train")
	float InitialSplineInputKey = 0.f;

	UPROPERTY(EditAnywhere, Category = "Train")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Train")
	float LookSensitivity = 1.f;

	// 상하 조준 각도 제한
	UPROPERTY(EditAnywhere, Category = "Train")
	float MinPitch = -60.f;

	UPROPERTY(EditAnywhere, Category = "Train")
	float MaxPitch = 60.f;

	// ===== 전투 설정 (플레이어가 직접 조작하는 터렛) =====
	// ★ 히트스캔 방식이라 탄환 액터를 쓰지 않음 (ProjectileClass/Speed 불필요)

	UPROPERTY(EditAnywhere, Category = "Train | Combat")
	float Damage = 25.f;

	// ★ 사거리. 이 거리 안에서만 맞음. 히트스캔이라 탄환이 날아가는 시간 없이
	// 발사 즉시 판정됨 (1인칭 조준에 맞춰 즉발 방식으로 처리).
	UPROPERTY(EditAnywhere, Category = "Train | Combat")
	float FireRange = 5000.f;

	// ★ 판정 굵기. 선이 아니라 이 반경의 구체를 쏘기 때문에 조준이 살짝
	// 빗나가도 맞음. 키우면 쉬워지고 줄이면 정밀 조준이 필요해짐.
	UPROPERTY(EditAnywhere, Category = "Train | Combat")
	float TraceRadius = 30.f;

	// 발사 간격(초). 버튼을 누르고 있으면 이 간격으로 계속 발사됨.
	// 업그레이드로 이 값을 줄이면 공격 속도가 빨라지는 구조.
	UPROPERTY(EditAnywhere, Category = "Train | Combat")
	float FireInterval = 2.f;

	// ★ 발사하는 순간 발생. 블루프린트에서 이 이벤트를 받아 머즐 플래시,
	// 발사음, 카메라 흔들림 등을 붙이면 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Train | Combat")
	void OnTurretFired(const FVector& MuzzleLocation, const FRotator& MuzzleRotation);

	// ★ 무언가에 명중한 순간 발생. 맞은 지점과 표면 방향을 넘겨주므로,
	// 블루프린트에서 임팩트 이펙트를 그 자리에 붙이면 됨.
	// bHitMonster로 몬스터인지 지형인지 구분되니, 분기해서 피 이펙트와
	// 일반 충돌 이펙트를 따로 쓸 수 있음.
	UFUNCTION(BlueprintImplementableEvent, Category = "Train | Combat")
	void OnTurretHit(const FVector& HitLocation, const FVector& HitNormal, AActor* HitActor, bool bHitMonster);

	// ★ 아무것도 못 맞추고 사거리 끝까지 간 경우 발생 (허공에 쏜 경우).
	// 필요 없으면 블루프린트에서 구현 안 해도 됨.
	UFUNCTION(BlueprintImplementableEvent, Category = "Train | Combat")
	void OnTurretMissed(const FVector& EndLocation);

	// ===== 입력 (Enhanced Input) =====

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	// 기차에 타고 있는 동안 하차 키를 누르면 이 액션으로 처리
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> DismountAction;

	// ★ 레일 분기(교차 지점에서 다른 레일로 갈아타기) 액션. Q키 권장.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SwitchRailAction;

	// 지금 분기 가능한 상태인지 (범위 안에 분기점이 있는지). UI에서 "Q키를
	// 누르세요" 안내를 띄울지 판단할 때 블루프린트에서 읽어감.
	UPROPERTY(BlueprintReadOnly, Category = "Train")
	bool bCanSwitchRail = false;

	// ★ 보스의 도끼 등에 맞았을 때 레일 위에서 뒤로 밀려나게 함.
	// 순간이동처럼 보이지 않도록 여러 프레임에 걸쳐 부드럽게 이동시킴.
	UFUNCTION(BlueprintCallable, Category = "Train")
	void ApplyKnockback(float Distance, float Duration);

	// ★ 보스의 "전쟁의 함성" 등에 걸리면 이동이 크게 느려지고 시야가 좁아짐.
	// 완전히 못 움직이게 하면 답답하므로, 느려지는 정도로만 제약을 둠.
	UFUNCTION(BlueprintCallable, Category = "Train")
	void ApplyStun(float Duration);

	// 스턴 중 이동 속도 배율 (0.2면 평소의 20% 속도)
	UPROPERTY(EditAnywhere, Category = "Train")
	float StunSpeedMultiplier = 0.2f;

	// 스턴 중에 시야가 좁아지는 정도 (기본 FOV에서 이만큼 뺌)
	UPROPERTY(EditAnywhere, Category = "Train")
	float StunFOVReduction = 10.f;

	// FOV가 변하는 속도 (클수록 빠르게 전환)
	UPROPERTY(EditAnywhere, Category = "Train")
	float StunFOVInterpSpeed = 4.f;

	// 스턴 중인지 (애니메이션이나 UI에서 참고 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Train")
	bool bIsStunned = false;

	// ===== 탑승/하차 =====

	// 지금 이 기차에 탑승 중인 폰(보통 플레이어 캐릭터). 하차 시 다시 이 폰으로 Possess함.
	UPROPERTY(BlueprintReadOnly, Category = "Train")
	TObjectPtr<APawn> MountedPawn;

	// 캐릭터가 기차에 상호작용했을 때 호출 (캐릭터 쪽 상호작용 함수에서 불러줄 것)
	UFUNCTION(BlueprintCallable, Category = "Train")
	void MountPawn(APawn* PawnToMount);

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Fire(const FInputActionValue& Value);
	void OnDismountPressed(const FInputActionValue& Value);

	// 분기 키를 눌렀을 때: 범위 안에 분기점이 있으면 반대편 레일로 갈아탐
	void OnSwitchRailPressed(const FInputActionValue& Value);

	// 지금 범위 안에 있는 분기점을 찾음 (없으면 nullptr)
	ARailSwitchPoint* FindNearbySwitchPoint() const;

	void Dismount();

	// ★ 하차 위치를 다음 프레임에 확정하는 함수. CharacterMovementComponent의
	// 발판 위치 복원이 전부 끝난 뒤에 실행되도록 타이머로 지연시킴.
	void FinalizeDismountPosition();

	UPROPERTY()
	TObjectPtr<APawn> PendingDismountPawn;

	FVector PendingDismountLocation = FVector::ZeroVector;
	float PendingDismountYaw = 0.f;

	// ★ 넉백 진행 상태. 남은 거리를 매 프레임 조금씩 소진시켜서
	// 순간이동이 아니라 밀려나는 느낌이 나게 함.
	float KnockbackRemaining = 0.f;
	float KnockbackSpeed = 0.f;

	// 스턴 해제용 타이머와 FOV 관련 값
	FTimerHandle StunTimerHandle;
	float OriginalFOV = 90.f;
	float TargetFOV = 0.f; // 0이면 아직 스턴을 겪지 않은 상태 (보간 안 함)

	void EndStun();

private:
	float FireCooldownRemaining = 0.f;
};