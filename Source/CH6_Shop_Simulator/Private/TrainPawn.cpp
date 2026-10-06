// Fill out your copyright notice in the Description page of Project Settings.

#include "TrainPawn.h"
#include "SplineFollowComponent.h"
#include "RailSwitchPoint.h"
#include "Engine/DamageEvents.h"
#include "MonsterBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

ATrainPawn::ATrainPawn()
{
	// 발사 쿨다운을 시간에 따라 줄여야 해서 Tick 필요
	PrimaryActorTick.bCanEverTick = true;

	// ★ 눈에 안 보이는 빈 컴포넌트를 진짜 루트로 두고, TrainMesh는 그 밑에 자식으로 붙임.
	// TrainMesh 자체를 루트로 쓰면 "액터 좌표계의 기준점"이 되어버려서 에디터에서
	// 위치/크기를 자유롭게 조정하기 애매해지는 문제가 있었음.
	TrainRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TrainRoot"));
	RootComponent = TrainRoot;
	TrainRoot->SetMobility(EComponentMobility::Movable);

	TrainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrainMesh"));
	TrainMesh->SetupAttachment(RootComponent);
	// ProjectileMovementComponent 때와 같은 이유로 Movable이어야 함
	TrainMesh->SetMobility(EComponentMobility::Movable);

	SplineFollowComp = CreateDefaultSubobject<USplineFollowComponent>(TEXT("SplineFollowComp"));

	// ★ Absolute Rotation: 기차 몸체(TrainMesh) 회전과 완전히 분리됨.
	// 코너를 돌 때 몸체는 레일 방향대로 꺾이지만, 터렛(=플레이어 조준 방향)은
	// 그 영향을 안 받고 마우스로 조작한 각도 그대로 유지됨.
	TurretHeadComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretHeadComp"));
	TurretHeadComp->SetupAttachment(RootComponent);
	TurretHeadComp->SetUsingAbsoluteRotation(true);
	TurretHeadComp->SetRelativeLocation(FVector(0.f, 0.f, 80.f)); // 기차 크기에 맞게 조정

	// ★ 실제로 보이는 터렛 메시. TurretHeadComp(조준 피벗)의 자식이라 조준 회전은
	// 그대로 따라가지만, 이 컴포넌트 자신의 회전은 코드가 안 건드려서 모델의
	// 정면 축이 안 맞을 때 여기서 수동으로 보정 가능.
	TurretVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretVisualMesh"));
	TurretVisualMesh->SetupAttachment(TurretHeadComp);

	// 카메라를 터렛에 붙여서, 조준 방향이 곧 플레이어 시야가 되게 함
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(TurretHeadComp);

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(TurretHeadComp);
	MuzzlePoint->SetRelativeLocation(FVector(80.f, 0.f, 0.f)); // 터렛 앞쪽 끝

	RearCouplingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RearCouplingPoint"));
	RearCouplingPoint->SetupAttachment(RootComponent);
	RearCouplingPoint->SetRelativeLocation(FVector(-200.f, 0.f, 0.f)); // 기차 크기에 맞게 조정

	// ★ 하차 위치 (터렛 바로 뒤). 뷰포트에서 갑판 위 적당한 자리로 조정할 것.
	DismountPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DismountPoint"));
	DismountPoint->SetupAttachment(RootComponent);
	DismountPoint->SetRelativeLocation(FVector(-120.f, 0.f, 100.f)); // 터렛 뒤쪽, 갑판 높이

	// 탑승/하차 시스템은 나중에 만들 예정. APawn의 AutoPossessPlayer 기본값이
	// 이미 Disabled라서 따로 설정 안 해도 자동 빙의는 안 됨.
}

void ATrainPawn::BeginPlay()
{
	Super::BeginPlay();

	// ★ 매핑 컨텍스트 등록은 여기서 하지 않음! BeginPlay 시점엔 레벨에 배치된
	// 기차가 아직 아무한테도 빙의 안 된 상태라 GetController()가 항상 null임.
	// 실제로 빙의되는 순간(PossessedBy)에 등록하도록 옮김.

	// ★ Default Pawn Class로 자동 스폰되면(레벨에 미리 배치해둔 인스턴스가 아니라
	// 새 인스턴스가 생기는 경우), RailSplineActor 수동 지정이 비어있을 수 있음.
	// 그럴 땐 "TrainRail" 태그가 붙은 액터를 자동으로 찾아서 대신 씀.
	if (!RailSplineActor)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("TrainRail")))
			{
				RailSplineActor = *It;
				break;
			}
		}
	}

	if (RailSplineActor && SplineFollowComp)
	{
		SplineFollowComp->TargetSpline = RailSplineActor->FindComponentByClass<USplineComponent>();

		if (SplineFollowComp->TargetSpline)
		{
			// ★ 인풋 키(조절점 번호)를 실제 누적 거리(cm)로 변환해서 사용.
			// 이러면 "몇 cm 지점"이 아니라 "몇 번째 조절점"으로 직관적으로 지정 가능.
			const float InitialDistance =
				SplineFollowComp->TargetSpline->GetDistanceAlongSplineAtSplineInputKey(InitialSplineInputKey);
			SplineFollowComp->SetDistance(InitialDistance);
		}
	}
}

void ATrainPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ATrainPawn::UnPossessed()
{
	// 하차할 때 이 기차의 매핑 컨텍스트를 제거. 안 그러면 캐릭터로 돌아가도
	// 기차 조작 매핑이 계속 겹쳐서 남아있게 됨.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->RemoveMappingContext(DefaultMappingContext);
			}
		}
	}

	Super::UnPossessed();
}

void ATrainPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATrainPawn::Move);
		}
		if (LookAction)
		{
			EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATrainPawn::Look);
		}
		if (FireAction)
		{
			// ★ Triggered: 버튼을 누르고 있는 동안 매 프레임 호출됨.
			// 실제 발사 간격은 Fire() 안의 FireCooldownRemaining이 제어하므로,
			// FireInterval 값만 바꾸면 연사 속도를 조절할 수 있음
			// (나중에 업그레이드로 이 값을 줄이면 자연스럽게 공속이 빨라짐).
			EnhancedInput->BindAction(FireAction, ETriggerEvent::Triggered, this, &ATrainPawn::Fire);
		}
		if (DismountAction)
		{
			EnhancedInput->BindAction(DismountAction, ETriggerEvent::Started, this, &ATrainPawn::OnDismountPressed);
			EnhancedInput->BindAction(SwitchRailAction, ETriggerEvent::Started, this, &ATrainPawn::OnSwitchRailPressed);
		}
	}
}

void ATrainPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FireCooldownRemaining > 0.f)
	{
		FireCooldownRemaining -= DeltaTime;
	}

	// ★ 스턴 시 시야각을 부드럽게 좁혔다가 되돌림
	if (TargetFOV > 0.f && FirstPersonCamera
		&& !FMath::IsNearlyEqual(FirstPersonCamera->FieldOfView, TargetFOV, 0.1f))
	{
		const float NewFOV = FMath::FInterpTo(
			FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, StunFOVInterpSpeed);
		FirstPersonCamera->SetFieldOfView(NewFOV);
	}

	// ★ 넉백 처리. 남은 거리를 매 프레임 조금씩 소진시켜서 부드럽게 밀려나게 함.
	// (한 번에 거리를 빼버리면 순간이동처럼 보임)
	if (KnockbackRemaining > 0.f && SplineFollowComp)
	{
		const float StepDistance = FMath::Min(KnockbackSpeed * DeltaTime, KnockbackRemaining);
		const float NewDistance = FMath::Max(0.f, SplineFollowComp->DistanceAlongSpline - StepDistance);

		SplineFollowComp->SetDistance(NewDistance);
		KnockbackRemaining -= StepDistance;

		// 스플라인 시작점에 닿으면 더 밀릴 곳이 없으므로 중단
		if (NewDistance <= 0.f)
		{
			KnockbackRemaining = 0.f;
		}
	}

	// ★ 지금 분기 가능한지 매 프레임 갱신. 블루프린트에서 이 값을 보고
	// "Q키를 누르세요" 안내 UI를 띄웠다 없앴다 하면 됨.
	bCanSwitchRail = (FindNearbySwitchPoint() != nullptr);

	// 정식 UMG 위젯 만들기 전 임시 안내. 나중에 블루프린트에서 bCanSwitchRail을
	// 읽어서 예쁜 UI로 교체하면 이 부분은 지워도 됨.
	if (bCanSwitchRail)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, TEXT("[Q] 레일 전환"));
	}
}

ARailSwitchPoint* ATrainPawn::FindNearbySwitchPoint() const
{
	if (!SplineFollowComp || !SplineFollowComp->TargetSpline || !GetWorld())
	{
		return nullptr;
	}

	// 레벨에 배치된 분기점들 중, 지금 기차가 그 범위 안에 있으면서
	// 현재 타고 있는 레일과 연결된 것을 찾음
	for (TActorIterator<ARailSwitchPoint> It(GetWorld()); It; ++It)
	{
		ARailSwitchPoint* SwitchPoint = *It;
		if (!SwitchPoint)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), SwitchPoint->GetActorLocation());
		if (DistSq > FMath::Square(SwitchPoint->GetSwitchRadius()))
		{
			continue; // 범위 밖
		}

		// 범위 안이어도, 지금 타고 있는 레일이랑 연결된 분기점이어야 의미가 있음
		USplineComponent* TargetSpline = nullptr;
		float TargetDistance = 0.f;
		if (SwitchPoint->GetOppositeRail(SplineFollowComp->TargetSpline, TargetSpline, TargetDistance))
		{
			return SwitchPoint;
		}
	}

	return nullptr;
}

void ATrainPawn::OnSwitchRailPressed(const FInputActionValue& Value)
{
	ARailSwitchPoint* SwitchPoint = FindNearbySwitchPoint();
	if (!SwitchPoint || !SplineFollowComp)
	{
		return;
	}

	USplineComponent* TargetSpline = nullptr;
	float TargetDistance = 0.f;
	if (SwitchPoint->GetOppositeRail(SplineFollowComp->TargetSpline, TargetSpline, TargetDistance))
	{
		// ★ 반대편 레일로 갈아탐. 스플라인과 그 위의 거리를 동시에 바꿔줘야
		// 기차가 엉뚱한 곳으로 순간이동하지 않음.
		SplineFollowComp->TargetSpline = TargetSpline;
		SplineFollowComp->SetDistance(TargetDistance);
	}
}

void ATrainPawn::ApplyKnockback(float Distance, float Duration)
{
	if (Distance <= 0.f)
	{
		return;
	}

	// 이미 밀려나는 중이면 거리를 누적 (연속으로 맞으면 더 많이 밀림)
	KnockbackRemaining += Distance;

	// 주어진 시간 안에 다 밀려나도록 속도를 계산
	const float SafeDuration = FMath::Max(0.1f, Duration);
	KnockbackSpeed = Distance / SafeDuration;
}

void ATrainPawn::ApplyStun(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	// 처음 걸릴 때만 원래 FOV를 기억 (연속으로 맞아도 값이 누적되지 않게)
	if (!bIsStunned && FirstPersonCamera)
	{
		OriginalFOV = FirstPersonCamera->FieldOfView;
	}

	bIsStunned = true;
	TargetFOV = OriginalFOV - StunFOVReduction; // Tick에서 부드럽게 따라감

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &ATrainPawn::EndStun, Duration, false);
}

void ATrainPawn::EndStun()
{
	bIsStunned = false;
	TargetFOV = OriginalFOV;
}

void ATrainPawn::Move(const FInputActionValue& Value)
{
	// 스턴 중에는 조작 불가 (보스의 "전쟁의 함성" 등)
	if (!SplineFollowComp || !GetWorld())
	{
		return;
	}

	// W/S -> 전진/후진. -1(후진) ~ 1(전진)
	const float AxisValue = Value.Get<float>();
	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	// 스턴 중에는 완전히 멈추는 대신 크게 느려짐 (조작 자체는 살려둠)
	const float CurrentSpeed = bIsStunned ? (MoveSpeed * StunSpeedMultiplier) : MoveSpeed;
	SplineFollowComp->AdvanceByDelta(AxisValue * CurrentSpeed * DeltaTime);

	// ★ 스플라인 탄젠트에 의도치 않은 Pitch/Roll이 섞여 있으면(에디터에서 조절점이
	// 완벽히 수평이 아닐 때 흔히 생김), 거기 부착된 DismountPoint 등 자식 컴포넌트
	// 위치가 이상한 곳으로 튀는 문제가 생김. 기차 몸체는 좌우 회전(Yaw)만 하도록
	// 강제해서 이 문제를 원천적으로 막음.
	const FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FRotator(0.f, CurrentRotation.Yaw, 0.f));
}

void ATrainPawn::Look(const FInputActionValue& Value)
{
	if (!TurretHeadComp)
	{
		return;
	}

	// 마우스 -> 터렛 조준 회전. 카메라가 터렛에 붙어있어서 이게 곧 플레이어 시야가 됨.
	// ★ 좌우(Yaw)는 제한하지 않음. 기차가 코너를 돌거나 다른 스플라인으로 갈아탈 때
	// 제한 기준(몸체 방향)이 같이 회전해버려서, 조준이 억지로 끌려가며 뚝 끊기는
	// 현상이 있었음. 상하(Pitch)만 제한해서 위아래로 과하게 꺾이는 것만 방지.
	const FVector2D LookAxis = Value.Get<FVector2D>();

	FRotator NewRotation = TurretHeadComp->GetRelativeRotation();
	NewRotation.Yaw += LookAxis.X * LookSensitivity;
	NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch - LookAxis.Y * LookSensitivity, MinPitch, MaxPitch);
	TurretHeadComp->SetRelativeRotation(NewRotation);
}

void ATrainPawn::Fire(const FInputActionValue& Value)
{
	if (!MuzzlePoint || FireCooldownRemaining > 0.f || !GetWorld())
	{
		return;
	}

	FireCooldownRemaining = FireInterval;

	const FVector MuzzleLocation = MuzzlePoint->GetComponentLocation();

	// ★ 판정은 머즐이 아니라 카메라에서 시작함. 머즐에서 쏘면 난간이나 성벽
	// 안쪽에 있어서 총구 바로 앞이 막혀버리는 문제가 있었음. 카메라 기준으로
	// 하면 "화면에 보이는 것 = 맞출 수 있는 것"이 되어 조준감도 정확해짐.
	// (시각 이펙트는 그대로 머즐에서 나가므로 보기에는 자연스러움)
	const FVector TraceStart = FirstPersonCamera
		? FirstPersonCamera->GetComponentLocation() : MuzzleLocation;
	const FVector Direction = FirstPersonCamera
		? FirstPersonCamera->GetForwardVector() : MuzzlePoint->GetForwardVector();
	const FVector TraceEnd = TraceStart + (Direction * FireRange);

	// 발사 연출(머즐 플래시, 사운드 등)은 블루프린트에서 이 이벤트를 받아 처리
	OnTurretFired(MuzzleLocation, Direction.Rotation());

	// ★ 탄환 액터를 만들지 않고 라인트레이스로 즉시 판정함.
	// 1인칭 조준이라 탄환이 날아가는 과정이 거의 안 보이는데다, 속도를 높이면
	// 한 프레임에 너무 멀리 이동해서 얇은 대상을 뚫고 지나갈 수 있기 때문.
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (MountedPawn)
	{
		// 갑판 위에 서있는 캐릭터 자신은 맞지 않게 무시
		QueryParams.AddIgnoredActor(MountedPawn);
	}

	// ★ 선 대신 구체를 쏨. 1인칭에서 움직이는 몬스터를 정확히 맞추기 어려워서,
	// 약간의 조준 오차는 허용되도록 굵기를 줌 (TraceRadius로 조절).
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(TraceRadius), QueryParams);

	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();

		// 맞은 게 몬스터인지 구분해서 넘겨줌 (피 이펙트 / 일반 충돌 이펙트 분기용)
		const bool bHitMonster = HitActor && HitActor->IsA(AMonsterBase::StaticClass());

		if (HitActor)
		{
			FDamageEvent DamageEvent;
			HitActor->TakeDamage(Damage, DamageEvent, GetController(), this);
		}

		// 임팩트 연출은 블루프린트에서 이 이벤트를 받아 처리
		OnTurretHit(Hit.ImpactPoint, Hit.ImpactNormal, HitActor, bHitMonster);
	}
	else
	{
		OnTurretMissed(TraceEnd);
	}
}

void ATrainPawn::MountPawn(APawn* PawnToMount)
{
	if (!PawnToMount)
	{
		return;
	}

	AController* PC = PawnToMount->GetController();
	if (!PC)
	{
		return;
	}

	MountedPawn = PawnToMount;

	// ★ 숨기거나 콜리전을 끄지 않음! 캐릭터를 계속 갑판 위에 물리적으로 서있게
	// 둬야 CharacterMovementComponent의 "움직이는 발판 위에 서면 같이 이동"
	// 기능이 계속 작동함. 콜리전을 꺼버리면 캐릭터가 그 자리에 얼어붙어서
	// 기차만 혼자 이동해버리고, 하차할 때마다 위치를 억지로 계산해야 했음.
	PC->Possess(this);
}

void ATrainPawn::Dismount()
{
	if (!MountedPawn)
	{
		return;
	}

	AController* PC = GetController();
	if (!PC)
	{
		return;
	}

	// ★ 하차 위치 계산. 예전엔 "캐릭터가 탑승 내내 갑판 위에 서있었을 것"이라
	// 가정하고 그 자리 그대로 Possess만 했는데, 실제로는 갑판에서 미끄러지거나
	// 멀리서 클릭해서 탄 경우 엉뚱한 곳에 내리는 문제가 있었음.
	FVector DismountLocation = FVector::ZeroVector;
	const bool bHasDismountPoint = (DismountPoint != nullptr);
	if (bHasDismountPoint)
	{
		DismountLocation = DismountPoint->GetComponentLocation();
	}

	APawn* PawnToRestore = MountedPawn;
	PC->Possess(PawnToRestore);
	MountedPawn = nullptr;

	// ★ 위치 지정을 "다음 프레임"으로 미룸. 탑승 중에 캐릭터는
	// CharacterMovementComponent의 "움직이는 발판 위에 서면 같이 이동" 기능으로
	// 기차를 따라 이동한 상태인데, Possess 직후에도 그 컴포넌트가 자기가 기억하던
	// 발판 기준 위치로 되돌리는 처리를 계속함. 같은 프레임에 위치를 잡으면 그
	// 복원에 덮어써져서 기차 이동 거리만큼 어긋남(2배로 간 것처럼 보임).
	// 타이머(0초)로 다음 프레임에 실행시키면 모든 복원이 끝난 뒤에 확정됨.
	if (bHasDismountPoint && PawnToRestore)
	{
		PendingDismountPawn = PawnToRestore;
		PendingDismountLocation = DismountLocation;
		PendingDismountYaw = GetActorRotation().Yaw;

		GetWorldTimerManager().SetTimerForNextTick(this, &ATrainPawn::FinalizeDismountPosition);
	}

	// ★ 하차하면 터렛을 기차 몸체 정면으로 되돌림. 아무도 안 타고 있는데
	// 터렛이 마지막 조준 방향으로 어정쩡하게 꺾여있으면 보기 안 좋으므로.
	// (Absolute Rotation을 쓰고 있어서, 월드 기준 값을 직접 넣어줌)
	if (TurretHeadComp)
	{
		FRotator BaseRotation = GetActorRotation();
		BaseRotation.Pitch = 0.f;
		BaseRotation.Roll = 0.f;
		TurretHeadComp->SetWorldRotation(BaseRotation);
	}
}

void ATrainPawn::FinalizeDismountPosition()
{
	if (!PendingDismountPawn)
	{
		return;
	}

	// 발판 추적으로 쌓인 속도/관성을 정리하고 최종 위치를 확정
	if (ACharacter* Character = Cast<ACharacter>(PendingDismountPawn))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->Velocity = FVector::ZeroVector;
		}
	}

	PendingDismountPawn->SetActorLocation(PendingDismountLocation, false, nullptr, ETeleportType::TeleportPhysics);
	PendingDismountPawn->SetActorRotation(FRotator(0.f, PendingDismountYaw, 0.f));

	PendingDismountPawn = nullptr;
}

void ATrainPawn::OnDismountPressed(const FInputActionValue& Value)
{
	Dismount();
}