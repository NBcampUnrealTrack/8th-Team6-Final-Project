#include "DOSScheduledCharacter.h"

#include "DOSCharacterScheduleComponent.h"
#include "DOSCharacterAnimSet.h"
#include "DOSTimelineSubsystem.h"
#include "Case/DOSCaseSubsystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/Navigation/NavigationTypes.h"

namespace
{
	// 웨이포인트가 죽음을 나타내는 태그. 웹 익스포터가 dead 플래그를 이 태그로 내보낸다.
	// (별도의 bDead 필드는 없다 - 태그 하나로 충분하다)
	const FGameplayTag& DeadActionTag()
	{
		static const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Action.Dead"), /*ErrorIfNotFound*/ false);
		return Tag;
	}
}

ADOSScheduledCharacter::ADOSScheduledCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Schedule = CreateDefaultSubobject<UDOSCharacterScheduleComponent>(TEXT("Schedule"));

	// 스스로 걸으려면 AI 컨트롤러가 붙어 있어야 한다.
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		// 웹 프로토타입 기준: 평상시 걸음이 타일당 190ms, 플레이어가 150ms다. 즉 인물은
		// 플레이어보다 조금 느리다 - 그 관계를 여기서도 유지한다(플레이어 220).
		Move->MaxWalkSpeed = 175.f;
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 360.f, 0.f);
		Move->MaxStepHeight = 45.f;
		Move->SetWalkableFloorAngle(50.f);

		// 경로 이동을 가속도로 굴리는 설정은 BeginPlay에 있다 - 여기(생성자)에 두면
		// 이미 저장된 BP가 값을 받지 못한다. NavMovementProperties 주석 참고.
	}
	bUseControllerRotationYaw = false;
}

void ADOSScheduledCharacter::BeginPlay()
{
	Super::BeginPlay();

	// **AI 경로 이동을 가속도로 굴린다.**
	//
	// 이게 꺼져 있으면(엔진 기본값 false) UPathFollowingComponent가
	// RequestDirectMove()로 **속도를 직접 밀어넣는다.** 인물은 움직이지만
	// CharacterMovement의 Acceleration 멤버는 손대지 않으므로
	// GetCurrentAcceleration()이 **언제나 0**이다.
	// ABP_Unarmed의 Should Move는 「지면 속도 있음 AND 가속 있음」이라서,
	// 걸어가는 내내 조건이 거짓이 되고 idle이 재생된다.
	//
	// 켜면 RequestPathMove(=AddInputVector)를 타서 가속도가 실제로 생긴다.
	//
	// 주의: 이건 UCharacterMovementComponent::bRequestedMoveUseAcceleration과
	// **다른 것이다.** 그쪽은 RequestedVelocity(RVO) 경로이고, CalcVelocity에서
	// 지역 변수로만 쓰여 Acceleration 멤버에 반영되지 않는다.
	//
	// 생성자가 아니라 BeginPlay에서 하는 이유: NavMovementProperties는 컴포넌트에
	// 직렬화되는 EditAnywhere 프로퍼티라, 이미 저장된 BP는 C++ 생성자 기본값 변경을
	// 받지 않는다. 여기서 넣으면 BP를 손대지 않아도 런타임에 확실히 적용된다.
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (FNavMovementProperties* NavProps = Move->GetNavMovementProperties())
		{
			NavProps->bUseAccelerationForPaths = true;
		}

		// 계단이 52° 경사면이라 여기서 올려준다. WalkableFloorAngleDegrees 주석 참고.
		// 반드시 세터를 쓴다 - WalkableFloorZ를 함께 갱신하지 않으면 값이 무시된다.
		// (생성자가 아니라 BeginPlay인 이유도 위와 같다: 이미 저장된 BP가 값을 덮는다.)
		Move->SetWalkableFloorAngle(WalkableFloorAngleDegrees);
	}

	// 경로 추종이 **왜** 끝났는지 받아 둔다. 이게 없으면 "계단 중간에 멈췄다"를 보고도
	// 도착으로 판단한 것인지 지오메트리에 막힌 것인지 구분할 수 없다.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->ReceiveMoveCompleted.AddUniqueDynamic(this, &ADOSScheduledCharacter::HandleMoveCompleted);
	}

	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{
		InitialMeshRelativeTransform = MeshComp->GetRelativeTransform();
		if (!MeshComp->GetPhysicsAsset())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[DOS] %s: 스켈레탈 메시에 Physics Asset이 없습니다 - 죽어도 쓰러지지 않습니다."),
				*CharacterId.ToString());
		}
	}
	if (!DeadActionTag().IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DOS] GameplayTag 'Action.Dead'가 등록되어 있지 않습니다 - 아무도 죽지 않습니다. ")
			TEXT("Config/DefaultGameplayTags.ini를 확인하세요."));
	}

	if (Schedule)
	{
		// 걷게 할 거라면 컴포넌트가 트랜스폼을 찍지 못하게 막는다 - 안 그러면 매 틱
		// 순간이동이 걸음을 덮어쓴다.
		Schedule->bApplyTransformToOwner = !bWalkWithNavigation;
		Schedule->OnCharacterStateApplied.AddDynamic(this, &ADOSScheduledCharacter::HandleStateApplied);
	}

	// 개입 상태를 일정표에 물린다. **이 배선이 System 2(분기)와 System 1(스케줄)을 잇는
	// 유일한 지점이다.** 컴포넌트 주석이 "소유 액터가 책임진다"고 적어둔 그 자리인데
	// 실제로는 아무도 하고 있지 않았다.
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UDOSCaseSubsystem* Case = GameInstance->GetSubsystem<UDOSCaseSubsystem>())
			{
				CachedCaseSubsystem = Case;
				Case->OnActiveInterventionsChanged.AddDynamic(
					this, &ADOSScheduledCharacter::HandleInterventionsChanged);
				// 회차 도중에 스폰됐을 수도 있으니 지금 상태로 한 번 맞춰둔다.
				HandleInterventionsChanged();
			}
		}
	}
}

void ADOSScheduledCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get())
	{
		Case->OnActiveInterventionsChanged.RemoveDynamic(
			this, &ADOSScheduledCharacter::HandleInterventionsChanged);
	}
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SettleTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void ADOSScheduledCharacter::HandleInterventionsChanged()
{
	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	if (!Case || !Schedule)
	{
		return;
	}

	Schedule->RefreshResolvedSchedule(Case->GetActiveInterventions());

	// 갈래가 바뀌면 목표도 바뀐다. 캐시를 비워 다음 상태에서 경로를 새로 잡게 한다 -
	// 안 그러면 옛 목표와 거의 같다는 이유로 새 경로 요청이 건너뛰어진다.
	bHasTarget = false;
}

void ADOSScheduledCharacter::HandleStateApplied(const FDOSCharacterState& State)
{
	CurrentState = State;

	// 죽음은 걷기보다 먼저 본다. 시신은 경로 요청을 하지 않는다.
	const bool bShouldBeDead = DeadActionTag().IsValid() && State.ActionTag == DeadActionTag();
	if (bShouldBeDead)
	{
		if (CorpseState == EDOSCorpseState::Alive)
		{
			EnterRagdoll();
		}
		return;
	}
	if (CorpseState != EDOSCorpseState::Alive)
	{
		// 되감았거나, 개입이 바뀌어 죽지 않는 갈래로 옮겨갔다.
		Revive();
	}

	if (!bWalkWithNavigation || bNavigationUnavailable)
	{
		return;
	}

	// **보간값(State.Location)이 아니라 웨이포인트 자체(State.GoalLocation)를 목표로 삼는다.**
	// 보간값을 쫓으면 걸음(175)이 목표보다 빨라 즉시 따라잡고 멈추기를 반복해서, 속도가
	// 대부분 0이 되어 걷는 애니메이션이 안 나오고 회전도 왕복한다.
	// GoalLocation은 웨이포인트가 바뀔 때만 변하므로 경로 요청이 한 번씩만 나간다.
	if (!bHasTarget || FVector::DistSquared(PendingTarget, State.GoalLocation) > FMath::Square(AcceptanceRadius))
	{
		PendingTarget = State.GoalLocation;
		bHasTarget = true;
		if (!RequestWalkTo(PendingTarget))
		{
			// **경로 하나가 실패한 것으로 걷기를 영구히 포기하지 않는다.**
			// 종전에는 여기서 보간 배치로 되돌렸는데, 목표가 한 번 벽 속에 들어가면
			// 그 회차 내내 벽을 뚫고 미끄러졌고 걷는 애니메이션도 안 나왔다.
			// 다음 박자에 다시 시도한다.
			bHasTarget = false;

			// NavMesh 자체가 없는 레벨(블록아웃 전)에서는 아무도 안 움직이는 것보다
			// 미끄러지는 편이 낫고, "NavMesh를 깔아야 한다"는 신호로도 쓰인다.
			const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (!Nav)
			{
				bNavigationUnavailable = true;
				Schedule->bApplyTransformToOwner = true;
				UE_LOG(LogTemp, Warning,
					TEXT("[DOS] %s: 레벨에 NavMesh가 없습니다. 보간 배치로 되돌립니다 - NavMeshBoundsVolume을 놓으세요."),
					*CharacterId.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Verbose,
					TEXT("[DOS] %s: 이 목표로는 경로를 못 찾았습니다(%s). 다음 박자에 다시 시도합니다."),
					*CharacterId.ToString(), *PendingTarget.ToCompactString());
			}
		}
	}
}

bool ADOSScheduledCharacter::RequestWalkTo(const FVector& Target)
{
	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		return false;
	}

	// BeginPlay 시점에 컨트롤러가 아직 없었을 수도 있다(스폰 순서). AddUnique라 중복은 없다.
	AI->ReceiveMoveCompleted.AddUniqueDynamic(this, &ADOSScheduledCharacter::HandleMoveCompleted);

	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav)
	{
		return false;
	}

	// **목표를 NavMesh에 투영한다.** SampleAtTime은 웨이포인트 사이를 직선 보간하므로,
	// 주방(1층)에서 안방(2층)으로 가는 동안 목표 지점이 **벽 속이나 공중**을 지난다.
	// 그걸 그대로 요청하면 경로를 못 찾고, 그때마다 걷기를 포기하면 인물이 벽을 뚫고
	// 미끄러진다(걸음 속도가 0이라 걷는 애니메이션도 안 나온다).
	FVector Goal = Target;
	FNavLocation Projected;
	const bool bProjected = Nav->ProjectPointToNavigation(Target, Projected, NavProjectionExtent);
	if (bProjected)
	{
		Goal = Projected.Location;

		// 많이 끌려갔으면 웨이포인트 좌표가 걸을 수 없는 곳에 있다는 뜻이다. 그래도
		// **투영된 자리로 계속 걸어간다** - 여기서 이동을 거부하면 인물이 그 자리에
		// 영영 멈춰 서고(계단 중간에 정지), 다음 박자로도 넘어가지 못한다.
		// 어느 웨이포인트가 어긋났는지는 화면 디버그와 아래 경고로 알 수 있다.
		const float SnapDist = FVector::Dist(Target, Goal);
		if (SnapDist > MaxNavSnapDistance)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[DOS] %s: 웨이포인트 좌표가 NavMesh에서 %.0f 떨어져 있습니다(투영해서 진행). ")
				TEXT("`P`로 초록색을 보면서 걸을 수 있는 바닥에 다시 찍으세요. 좌표=%s"),
				*CharacterId.ToString(), SnapDist, *Target.ToCompactString());
		}
	}

	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Goal, AcceptanceRadius, /*bStopOnOverlap*/ false);

	switch (Result)
	{
	case EPathFollowingRequestResult::RequestSuccessful:
		LastWalkResult = bProjected ? TEXT("경로OK(투영)") : TEXT("경로OK");
		return true;
	case EPathFollowingRequestResult::AlreadyAtGoal:
		LastWalkResult = TEXT("이미도착");
		return true;
	default:
		// **여기서 조용히 넘어가면 인물이 그 자리에 영영 멈춰 선다.** 목표가 NavMesh 밖이거나
		// (투영 실패) 그 지점까지 길이 없는 경우다.
		LastWalkResult = bProjected ? TEXT("경로없음") : TEXT("투영실패(NavMesh 밖)");
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: 목표로 갈 수 없습니다 - %s / 목표=%s / 투영=%s"),
			*CharacterId.ToString(), *LastWalkResult,
			*Target.ToCompactString(), bProjected ? TEXT("성공") : TEXT("실패"));
		return false;
	}
}

void ADOSScheduledCharacter::HandleMoveCompleted(FAIRequestID /*RequestID*/, EPathFollowingResult::Type Result)
{
	switch (Result)
	{
	case EPathFollowingResult::Success:  LastMoveResult = TEXT("도착"); break;
	case EPathFollowingResult::Blocked:  LastMoveResult = TEXT("막힘"); break;
	case EPathFollowingResult::OffPath:  LastMoveResult = TEXT("경로이탈"); break;
	case EPathFollowingResult::Aborted:  LastMoveResult = TEXT("중단"); break;
	// Skipped는 5.x에서 Skipped_DEPRECATED가 되었고 Aborted로 통합되었다. 쓰지 않는다.
	default:                             LastMoveResult = TEXT("무효"); break;
	}

	// 도착이 아니면 이유를 남긴다. 목표에서 얼마나 떨어진 자리에서 끝났는지가 핵심이다 -
	// 계단 중간에서 「도착」이 나오면 목표 좌표가 계단으로 투영된 것이고,
	// 「막힘」이 나오면 지오메트리(계단 높이/콜리전)가 문제다.
	if (Result != EPathFollowingResult::Success)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: 이동이 %s로 끝났습니다. 목표=%s / 끝난자리=%s / 남은거리=%.0f"),
			*CharacterId.ToString(), *LastMoveResult,
			*PendingTarget.ToCompactString(), *GetActorLocation().ToCompactString(),
			FVector::Dist2D(GetActorLocation(), PendingTarget));
	}
}

// --- 애니메이션 --------------------------------------------------------------

UAnimMontage* ADOSScheduledCharacter::ResolveActionMontage() const
{
	// ① 그 박자만의 것. 빅토리아의 왈츠처럼 한 자리에만 쓰이는 동작.
	if (!CurrentState.ActionMontage.IsNull())
	{
		if (UAnimMontage* Specific = CurrentState.ActionMontage.LoadSynchronous())
		{
			return Specific;
		}
	}

	// ② 그 인물다운 기본. 앨리스의 "일하기"는 앨리스답게.
	if (AnimSet)
	{
		if (UAnimMontage* PerCharacter = AnimSet->Find(CurrentState.ActionTag))
		{
			return PerCharacter;
		}
	}

	// ③ 아무 인물이나 쓰는 것.
	if (SharedAnimSet)
	{
		return SharedAnimSet->Find(CurrentState.ActionTag);
	}
	return nullptr;
}

void ADOSScheduledCharacter::AdaptWalkSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!bAdaptWalkSpeedToSchedule || !Move || !bHasTarget)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UDOSTimelineSubsystem* Timeline =
		GameInstance ? GameInstance->GetSubsystem<UDOSTimelineSubsystem>() : nullptr;
	if (!Timeline)
	{
		return;
	}

	const float Remaining = CurrentState.GoalTimeSeconds - Timeline->GetCurrentTime();
	const float Distance = FVector::Dist(GetActorLocation(), PendingTarget);

	// 이미 도착했거나 시각이 지났으면 손대지 않는다.
	if (Distance <= AcceptanceRadius || Remaining <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 직선거리 기준이라 실제 경로보다 짧다. 계단·복도를 돌아가는 만큼을 여유로 곱해준다.
	const float NeededSpeed = (Distance * 1.25f) / Remaining;
	Move->MaxWalkSpeed = FMath::Clamp(NeededSpeed, MinAdaptedWalkSpeed, MaxAdaptedWalkSpeed);

	if (NeededSpeed > MaxAdaptedWalkSpeed)
	{
		// 상한으로도 못 닿는다 = 방이 너무 멀다. 배치를 고쳐야 하는 신호다.
		UE_LOG(LogTemp, Warning,
			TEXT("[DOS] %s: %.0f초 안에 %.0f를 가야 해서 초당 %.0f가 필요합니다(상한 %.0f). ")
			TEXT("방이 너무 멀거나 웨이포인트 시각이 촘촘합니다."),
			*CharacterId.ToString(), Remaining, Distance, NeededSpeed, MaxAdaptedWalkSpeed);
	}
}

// --- 목격 --------------------------------------------------------------------

void ADOSScheduledCharacter::CheckWitness()
{
	if (CurrentState.WitnessId.IsNone())
	{
		return; // 지금 박자는 목격 대상이 아니다.
	}

	UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();
	if (!Case || Case->HasDiscoveredClue(CurrentState.WitnessId))
	{
		return; // 이미 본 것은 다시 재지 않는다.
	}

	if (!IsVisibleToPlayer())
	{
		return;
	}

	Case->DiscoverClue(CurrentState.WitnessId);
	UE_LOG(LogTemp, Log, TEXT("[DOS] 목격: %s (%s)"),
		*CurrentState.WitnessId.ToString(), *CharacterId.ToString());
}

bool ADOSScheduledCharacter::IsVisibleToPlayer() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return false;
	}

	// 카메라 기준으로 잰다. 폰 위치가 아니라 **보고 있는 곳**이 기준이어야
	// "등 뒤에서 벌어진 일을 봤다"가 되지 않는다.
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	// 발밑이 아니라 가슴께를 겨냥한다 - 바닥이 조금 솟아 있어도 트레이스가 막히지 않는다.
	const FVector Target = GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector ToTarget = Target - ViewLocation;
	const float Distance = ToTarget.Size();

	if (Distance > WitnessRange || Distance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	// 화면 안인가.
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(WitnessHalfAngleDegrees));
	if (FVector::DotProduct(ViewRotation.Vector(), ToTarget / Distance) < MinDot)
	{
		return false;
	}

	// 가려져 있지 않은가. 벽 너머로 본 것은 목격이 아니다.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DOSWitness), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(this);
	if (const APawn* PlayerPawn = PC->GetPawn())
	{
		Params.AddIgnoredActor(PlayerPawn);
	}

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(
		Hit, ViewLocation, Target, ECC_Visibility, Params);

	return !bBlocked;
}

// --- 시신 --------------------------------------------------------------------

void ADOSScheduledCharacter::EnterRagdoll()
{
	CorpseState = EDOSCorpseState::Ragdoll;
	bHasFrozenPose = false;

	// 걷던 것을 먼저 멈춘다. 안 그러면 경로추종이 계속 밀어댄다.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	// NavMesh가 없어 보간 배치로 돌던 경우, 스케줄이 매 틱 시신을 순간이동시킨다.
	// 물리와 정면으로 싸우므로 죽은 동안에는 반드시 꺼야 한다.
	if (Schedule)
	{
		Schedule->bApplyTransformToOwner = false;
	}

	// 캡슐을 치워야 시신이 제 캡슐에 걸려 공중에 뜨지 않는다.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// 특정 뼈 이름에 기대지 않는다 - 스켈레톤이 바뀌면 조용히 아무 일도 안 일어난다.
		MeshComp->SetSimulatePhysics(true);
		MeshComp->WakeAllRigidBodies();
	}

	// 자세가 안정되면 스냅샷으로 붙잡는다.
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			SettleTimer, this, &ADOSScheduledCharacter::FreezePose, SettleSeconds, false);
	}
}

void ADOSScheduledCharacter::FreezePose()
{
	if (CorpseState != EDOSCorpseState::Ragdoll)
	{
		return; // 그 사이에 되살아났다.
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// 지금 무너져 있는 자세를 통째로 뜬다. AnimBP의 Pose Snapshot 노드가 이걸 재생하므로
	// 물리를 꺼도 서 있는 자세로 돌아가지 않는다.
	MeshComp->SnapshotPose(FrozenPose);
	bHasFrozenPose = true;

	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	CorpseState = EDOSCorpseState::Frozen;
}

void ADOSScheduledCharacter::Revive()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SettleTimer);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MeshComp->SetCollisionObjectType(ECC_Pawn);
		// 물리가 가져갔던 트랜스폼을 BeginPlay에서 받아둔 값으로 되돌린다.
		MeshComp->SetRelativeTransform(InitialMeshRelativeTransform);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Walking);
	}
	if (Schedule)
	{
		Schedule->bApplyTransformToOwner = !bWalkWithNavigation;
	}

	bHasFrozenPose = false;
	CorpseState = EDOSCorpseState::Alive;
	bHasTarget = false; // 다음 상태에서 경로를 새로 잡는다.
}

void ADOSScheduledCharacter::BeginDragged(FName HeldBoneName)
{
	// TODO(발표 후): 잡힌 뼈만 kinematic으로 돌리고 나머지는 시뮬레이션에 둔다.
	//   MeshComp->SetSimulatePhysics(true);
	//   MeshComp->SetAllBodiesBelowSimulatePhysics(HeldBoneName, true, false);
	//   MeshComp->SetBodySimulatePhysics(HeldBoneName, false);   // 잡힌 곳만 고정
	// 그러면 팔·머리·다리가 중력에 매달려 끌린다. 드는 것(carry)은 전용 자세가 필요하지만
	// 끄는 것(drag)은 평상시 걸음으로도 읽힌다.  → Docs/Demo_Plan_t2.md 5절
	UE_LOG(LogTemp, Warning, TEXT("[DOS] BeginDragged는 아직 구현되지 않았습니다 (%s)"), *HeldBoneName.ToString());
}

void ADOSScheduledCharacter::EndDragged()
{
	// TODO(발표 후): 전신 시뮬레이션으로 되돌렸다가 안정되면 FreezePose().
	UE_LOG(LogTemp, Warning, TEXT("[DOS] EndDragged는 아직 구현되지 않았습니다"));
}

bool ADOSScheduledCharacter::IsWalking() const
{
	if (!bWalkWithNavigation || bNavigationUnavailable)
	{
		return false;
	}
	if (const AAIController* AI = Cast<AAIController>(GetController()))
	{
		return AI->GetMoveStatus() == EPathFollowingStatus::Moving;
	}
	return false;
}

void ADOSScheduledCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 목격 판정은 시신에도 돈다 - 쓰러진 것을 본 것도 목격이다.
	WitnessCheckAccumulator += DeltaSeconds;
	if (WitnessCheckAccumulator >= WitnessCheckInterval)
	{
		WitnessCheckAccumulator = 0.f;
		CheckWitness();
	}

	if (CorpseState != EDOSCorpseState::Alive)
	{
		return; // 시신은 방향을 고쳐 잡지 않는다.
	}

	// 목표가 있는데 도착도 못 했고 걷지도 않으면 **멈춰 선 것**이다. 주기적으로 다시 시도한다.
	if (bHasTarget && bWalkWithNavigation && !bNavigationUnavailable && !IsWalking())
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), PendingTarget);
		if (DistSq > FMath::Square(AcceptanceRadius * 1.5f))
		{
			WalkRetryAccumulator += DeltaSeconds;
			if (WalkRetryAccumulator >= WalkRetryInterval)
			{
				WalkRetryAccumulator = 0.f;
				RequestWalkTo(PendingTarget);
			}
		}
	}

	AdaptWalkSpeed();

	if (bShowWalkDebug && GEngine)
	{
		const float Speed = GetVelocity().Size();
		// **가속도**가 핵심이다. ABP_Unarmed의 Should Move가 속도와 가속도를 함께 보므로,
		// 속도가 있는데 가속도가 0이면 걷는 애니메이션이 안 나온다(AI 경로 이동이 가속도를
		// 쓰지 않고 속도를 직접 넣는 경우가 그렇다).
		const float Accel = GetCharacterMovement()
			? GetCharacterMovement()->GetCurrentAcceleration().Size2D() : 0.f;
		const float Dist = bHasTarget ? FVector::Dist(GetActorLocation(), PendingTarget) : -1.f;
		// CharacterId로 키를 갈라 인물마다 한 줄씩 제자리 갱신되게 한다.
		const int32 Key = 2000 + GetTypeHash(CharacterId) % 500;
		// **MovementMode가 핵심이다.** 계단에서 Falling이 되면 ABP의 Should Move가 거짓이
		// 되어 idle이 나오고, 경로추종도 멈춘다.
		const TCHAR* ModeText = TEXT("?");
		if (GetCharacterMovement())
		{
			switch (GetCharacterMovement()->MovementMode.GetValue())
			{
			case MOVE_None:    ModeText = TEXT("None"); break;
			case MOVE_Walking: ModeText = TEXT("Walking"); break;
			case MOVE_Falling: ModeText = TEXT("**Falling**"); break;
			case MOVE_Flying:  ModeText = TEXT("Flying"); break;
			default:           ModeText = TEXT("기타"); break;
			}
		}
		const float MaxSpeed = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 0.f;

		// **경로 추종의 현재 상태.** Idle이면 경로가 이미 끝난 것이고(LastMoveResult가 이유),
		// Moving인데 속도가 0이면 밀고 있는데 막힌 것이다. 부분경로면 목표까지 길이 없어
		// 갈 수 있는 데까지만 간 것이다 - 그게 계단에서 끝나면 NavMesh가 2층과 끊긴 것이다.
		FString PathText = TEXT("PF:-");
		if (const AAIController* AI = Cast<AAIController>(GetController()))
		{
			if (const UPathFollowingComponent* PFC = AI->GetPathFollowingComponent())
			{
				const TCHAR* StatusText = TEXT("?");
				switch (PFC->GetStatus())
				{
				case EPathFollowingStatus::Idle:    StatusText = TEXT("Idle"); break;
				case EPathFollowingStatus::Waiting: StatusText = TEXT("Waiting"); break;
				case EPathFollowingStatus::Paused:  StatusText = TEXT("Paused"); break;
				case EPathFollowingStatus::Moving:  StatusText = TEXT("Moving"); break;
				default: break;
				}
				const FNavPathSharedPtr Path = PFC->GetPath();
				PathText = FString::Printf(TEXT("PF:%s%s"), StatusText,
					(Path.IsValid() && Path->IsPartial()) ? TEXT("/**부분경로**") : TEXT(""));
			}
		}

		// **가속을 넣는데 안 나아가면 콜리전에 걸린 것이다.** 무엇에 걸렸는지 이름으로 찍는다 -
		// 계단 메시인지, 벽인지, 플레이어(유령이라 통과해야 한다)인지, 다른 인물인지가
		// 대책이 전혀 다르다. 「올라야할높이」가 MaxStepHeight를 넘으면 계단 챌면이 너무 높은 것이다.
		FString ProbeText;
		if (Accel > 1.f && Speed < 15.f && GetCharacterMovement() && GetWorld())
		{
			const UCapsuleComponent* Cap = GetCapsuleComponent();
			const float Radius = Cap->GetScaledCapsuleRadius();
			const float HalfHeight = Cap->GetScaledCapsuleHalfHeight();
			const FVector Dir = GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
			const FVector Start = GetActorLocation();
			const float FeetZ = Start.Z - HalfHeight;

			FHitResult Hit;
			FCollisionQueryParams Params(TEXT("DOSBlockProbe"), /*bTraceComplex*/ false, this);
			// 살짝 줄인 캡슐로 쏜다 - 원래 크기면 시작부터 바닥에 파묻혀 결과가 못 쓰게 된다.
			const bool bHit = GetWorld()->SweepSingleByChannel(
				Hit, Start, Start + Dir * (Radius + 30.f), FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(Radius * 0.95f, HalfHeight * 0.95f), Params);

			if (bHit)
			{
				// StepUp이 실패하는 조건은 셋뿐이다. 세 값을 나란히 찍어 어느 경계인지 못 놓치게 한다.
				//   ① 올라야할높이 > MaxStepHeight
				//   ② 부딪힌 면의 각도 > WalkableFloorAngle (챌면이 90°인 건 정상 - StepUp이 처리한다.
				//      문제는 **올라선 뒤 디딜 면**이 걸을 수 없을 때다)
				//   ③ 그 컴포넌트가 CanCharacterStepUp = No
				const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Hit.ImpactNormal.Z, -1.f, 1.f)));
				const bool bCanStepUpOn = Hit.GetComponent() ? Hit.GetComponent()->CanCharacterStepUp(this) : false;
				ProbeText = FString::Printf(
					TEXT("높이 %.0f/%.0f  면각도 %.0f/%.0f  StepUp %s  %s  법선Z %.2f  막은것 %s"),
					Hit.ImpactPoint.Z - FeetZ, GetCharacterMovement()->MaxStepHeight,
					SurfaceAngle, GetCharacterMovement()->GetWalkableFloorAngle(),
					bCanStepUpOn ? TEXT("O") : TEXT("**X**"),
					Hit.bStartPenetrating ? TEXT("**파묻힘**") : TEXT("파묻힘아님"),
					Hit.ImpactNormal.Z,
					*GetNameSafe(Hit.GetComponent()));
			}
			else
			{
				ProbeText = TEXT("막은것 없음 - 콜리전이 아니다");
			}
		}

		GEngine->AddOnScreenDebugMessage(Key, 0.f, FColor::Cyan,
			FString::Printf(TEXT("[%s] 거리 %.0f  속도 %.0f/%.0f  가속 %.0f  %s  %s  %s  끝=%s  %s  방=%s"),
				*CharacterId.ToString(), Dist, Speed, MaxSpeed, Accel, ModeText,
				IsWalking() ? TEXT("걷는중") : TEXT("정지"),
				*LastWalkResult, *LastMoveResult, *PathText,
				*CurrentState.RoomId.ToString()));

		// **막힘 진단은 별도 줄로 뺀다.** 한 줄에 붙이면 화면 폭에서 잘려 뒷부분이 안 보인다.
		// 노란색으로 갈라 두면 걷는 상태 줄과 구분된다.
		if (!ProbeText.IsEmpty())
		{
			GEngine->AddOnScreenDebugMessage(Key + 500, 0.f, FColor::Yellow,
				FString::Printf(TEXT("  └ %s: %s"), *CharacterId.ToString(), *ProbeText));
		}
	}

	// 도착한 뒤에는 스케줄이 지정한 방향을 본다 - 걸어온 방향 그대로 서 있으면 자리를 잡은
	// 것으로 보이지 않는다. 보간된 회전이 아니라 **웨이포인트가 지정한 방향**을 쓴다.
	if (bHasTarget && !IsWalking() && !CurrentState.GoalRotation.IsNearlyZero())
	{
		const FRotator Current = GetActorRotation();
		const FRotator Wanted(0.f, CurrentState.GoalRotation.Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(Current, Wanted, DeltaSeconds, 4.f));
	}
}
