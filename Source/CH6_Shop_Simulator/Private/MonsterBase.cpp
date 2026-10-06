// Fill out your copyright notice in the Description page of Project Settings.

#include "MonsterBase.h"
#include "HitTextActor.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "MaterialInventoryComponent.h"
#include "CraftingTypes.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "SplineFollowComponent.h"
#include "DefenseObjectBase.h"
#include "DefenseWall.h"
#include "SplinePathManager.h"
#include "BaseCore.h"
#include "SoldierUnit.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Components/WidgetComponent.h"
#include "HealthBarWidget.h"

AMonsterBase::AMonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(34.f, 88.f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	// 몬스터끼리는 서로 무시(Ignore)하고, 벽/바리케이드/바닥 같은 지형에는 그대로 막히게(Block) 함.
	// -> 이렇게 해야 좌우 오프셋을 줬을 때 앞 몬스터를 실제로 앞질러 갈 수 있음.
	// (Pawn 프로파일 기본값은 다른 Pawn도 Block이라 오프셋을 줘도 서로 못 지나갔던 것)
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	RootComponent = CapsuleComp;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜리전은 캡슐이 담당

	SplineFollowComp = CreateDefaultSubobject<USplineFollowComponent>(TEXT("SplineFollowComp"));

	// 체력바 UI: 머리 위에 항상 카메라를 향해 떠있게 설정
	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComp"));
	HealthBarWidgetComp->SetupAttachment(RootComponent);
	HealthBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 캡슐 크기에 맞게 조정
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 항상 카메라를 향함 (빌보드)
	HealthBarWidgetComp->SetDrawSize(FVector2D(100.f, 12.f));
}

void AMonsterBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	UpdateHealthBar(); // 시작하자마자 풀피 상태로 한 번 표시

	// ★ GroundOffsetZ를 캡슐의 실제 Half Height에서 자동으로 계산.
	// 엘리트 몬스터처럼 캡슐 크기를 키운 서브클래스/블루프린트에서도 숫자를 따로
	// 안 맞춰줘도 항상 정확하게 바닥 위에 서게 됨. (안 하면 캡슐이 바닥에 파묻혀서
	// 매 프레임 Sweep이 막혀 이동 자체가 안 되는 문제가 생김 - 실제로 겪었던 버그)
	if (CapsuleComp && SplineFollowComp)
	{
		SplineFollowComp->GroundOffsetZ = CapsuleComp->GetScaledCapsuleHalfHeight();
	}

	// ★ 블루프린트에서 콜리전 프리셋을 한 번이라도 만졌으면, 그 설정이 생성자보다 나중에
	// 적용되어 여기 설정을 덮어씌워버릴 수 있음. BeginPlay(블루프린트 기본값이 다 적용된
	// 다음 실행되는 시점)에서 다시 한번 강제로 걸어서, 몬스터끼리는 반드시 무시하게 함.
	// (벽/바리케이드는 그대로 막혀야 하므로 Pawn 채널만 건드림)
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// ★ 스켈레탈 메시가 피직스 애셋(뼈마다 캡슐)을 가지고 있으면, 콜리전이 켜져 있을 때
	// Object Type이 PhysicsBody로 참여해서 캡슐의 Pawn-Ignore 설정과 무관하게
	// 메시끼리 또 부딪혀서 막힐 수 있음. 이동/충돌 판정은 전부 CapsuleComp가 담당하므로
	// 메시 콜리전은 확실히 꺼둠.
	if (MeshComp)
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMonsterBase::InitializeMonster(USplineComponent* InSpline, float InHealthMultiplier, float InDamageMultiplier)
{
	if (SplineFollowComp)
	{
		SplineFollowComp->TargetSpline = InSpline;
		SplineFollowComp->SetDistance(0.f);
	}

	MaxHealth *= InHealthMultiplier;
	CurrentHealth = MaxHealth;
	AttackDamage *= InDamageMultiplier;

	CurrentState = EMonsterState::Moving;
	CurrentSegmentIndex = 0; // 첫 번째(0번) 킬존부터 확인 시작. INDEX_NONE(-1)이면 IsValidIndex가 항상 false라 아예 체크가 안 됨.
}

void AMonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ★ 매 프레임 계산할 필요 없이 주기적으로만 플레이어와의 거리를 확인해서
	// 체력바 표시 여부를 결정함. Dead/Stunned 여부와 무관하게 항상 체크되도록
	// 아래 early-return보다 위에 둠.
	HealthBarVisibilityCheckTimer += DeltaTime;
	if (HealthBarVisibilityCheckTimer >= HealthBarVisibilityCheckInterval)
	{
		HealthBarVisibilityCheckTimer = 0.f;

		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				const float DistSq = FVector::DistSquared(GetActorLocation(), PlayerPawn->GetActorLocation());
				bPlayerInHealthBarRange = (DistSq <= FMath::Square(HealthBarVisibleRange));
			}
		}
		RefreshHealthBarVisibility();
	}

	if (CurrentState == EMonsterState::Dead || bIsStunned)
	{
		return;
	}

	switch (CurrentState)
	{
	case EMonsterState::Moving:
		TickMoving(DeltaTime);
		break;
	case EMonsterState::Attacking:
		TickAttacking(DeltaTime);
		break;
	default:
		break;
	}
}

void AMonsterBase::TickMoving(float DeltaTime)
{
	// ★ 스킬 시전 중에는 제자리에 멈춤 (궁극기 몽타주 재생 등)
	if (bIsCasting)
	{
		return;
	}

	if (!SplineFollowComp || !SplineFollowComp->TargetSpline)
	{
		return;
	}

	SplineFollowComp->AdvanceByDelta(SplineFollowComp->BaseSpeed * DeltaTime);

	// 스플라인 끝(=BaseCore 앞)에 도달 -> 코어 공격 상태로 전환
	if (SplineFollowComp->HasReachedEnd())
	{
		if (!CurrentTarget)
		{
			// 코어는 레벨에 하나뿐이므로, 몬스터가 태어날 때 했던 것처럼 월드에서 찾아 연결.
			// (자주 있는 일이 아니라 - 몬스터가 스플라인 끝에 도달하는 순간에만 1회 실행되니
			// TActorIterator를 매 프레임 돌리는 것과는 다름. 성능 걱정 없음.)
			for (TActorIterator<ABaseCore> It(GetWorld()); It; ++It)
			{
				CurrentTarget = *It;
				break;
			}
		}
		CurrentState = EMonsterState::Attacking;
		AttackTimer = 0.f;
		return;
	}

	// 실제로 뭔가에 부딪혔는지(Sweep 충돌 결과)를 확인. 몬스터끼리는 콜리전을 무시하게
	// 해뒀으니, 여기서 걸리는 건 벽/바리케이드/코어처럼 진짜 막혀야 하는 대상뿐임.
	if (SplineFollowComp->LastMoveHit.bBlockingHit)
	{
		AActor* HitActor = SplineFollowComp->LastMoveHit.GetActor();

		if (ADefenseWall* Wall = Cast<ADefenseWall>(HitActor))
		{
			if (Wall->bIsActive)
			{
				CurrentTarget = Wall;
				CurrentState = EMonsterState::Attacking;
				AttackTimer = 0.f;
			}
		}
		else if (ABaseCore* Core = Cast<ABaseCore>(HitActor))
		{
			CurrentTarget = Core;
			CurrentState = EMonsterState::Attacking;
			AttackTimer = 0.f;
		}
		// ★ 그 외(병사 등 앞으로 생길 새로운 방어 유닛)는 개별 타입을 몰라도
		// ADefenseObjectBase 계열이면 자동으로 공격 대상으로 인식. 새 유닛 종류가
		// 늘어나도 여기 코드를 매번 추가할 필요 없음 (부모 클래스만 맞으면 됨).
		//
		// ★ 단, 병사(ASoldierUnit)는 예외. 병사는 ForceEngage()로 "명시적으로 지목된
		// 몬스터만" 교전하는 방식이라서, 그냥 스쳐 지나가다 부딪힌 것만으로 여기서
		// 자동으로 싸움이 시작되면 "1병사 1몬스터" 원칙이 깨져버림.
		else if (Cast<ASoldierUnit>(HitActor))
		{
			// 아무것도 안 함: 병사와의 교전 여부는 오직 ForceEngage()로만 결정됨
		}
		else if (ADefenseObjectBase* AnyDefenseUnit = Cast<ADefenseObjectBase>(HitActor))
		{
			CurrentTarget = AnyDefenseUnit;
			CurrentState = EMonsterState::Attacking;
			AttackTimer = 0.f;
		}
	}
}

void AMonsterBase::TickAttacking(float DeltaTime)
{
	// ★ 스킬 시전 중에는 일반 공격도 멈춤
	if (bIsCasting)
	{
		return;
	}

	// 대상이 아예 파괴(Destroy)됐거나, ADefenseWall인데 Deactivate()로 비활성화된 경우,
	// 또는 ASoldierUnit인데 이미 사망 몽타주 재생 중(반격 불가능한 상태)인 경우
	// 전부 "더 이상 막고 있지 않다"로 취급하고 이동 재개.
	// (ADefenseWall은 파괴돼도 Destroy가 아니라 Deactivate만 되므로 IsValid만으론 못 잡아냄 - bIsActive도 같이 체크)
	bool bTargetGone = !IsValid(CurrentTarget);
	if (!bTargetGone)
	{
		if (ADefenseWall* Wall = Cast<ADefenseWall>(CurrentTarget))
		{
			bTargetGone = !Wall->bIsActive;
		}
		else if (ASoldierUnit* Soldier = Cast<ASoldierUnit>(CurrentTarget))
		{
			// ★ 병사는 Destroy()가 몽타주 재생이 끝난 뒤로 지연되기 때문에, 죽어가는
			// 중(bIsDying)인데도 아직 IsValid라서 계속 때리는 문제가 있었음.
			bTargetGone = Soldier->IsDying();
		}
	}

	if (bTargetGone)
	{
		// 마지막 타격 애니메이션이 아직 재생 중이면, 그게 끝날 때까지 이동 재개를 미룸.
		// (안 그러면 공격 모션 중간에 뚝 끊기고 걸어가버리는 어색함이 생김)
		// 추가 메시(울프 라이더의 늑대/오크 등)도 같이 확인해서, 하나라도
		// 재생 중이면 기다림.
		if (IsAnyAttackMontagePlaying())
		{
			return;
		}

		CurrentTarget = nullptr;
		CurrentState = EMonsterState::Moving;
		CurrentSegmentIndex++; // 이 킬존은 처리 끝났으니 다음 킬존을 확인하도록 인덱스 전진
		return;
	}

	// ★ 공격 대상 방향으로 몸통을 돌리는 건 "병사(1:1 결투)"일 때만.
	// 벽/코어처럼 여러 메시가 넓게 퍼진 구조물은 "액터의 대표 위치(원점)"가
	// 실제로 부딪힌 지점이랑 다를 수 있어서, 이 회전을 적용하면 엉뚱한 방향
	// (구조물의 중앙 등)으로 돌아버리는 문제가 있었음. 벽/코어는 그냥 부딪힌
	// 순간의 진행 방향을 그대로 유지하는 게 항상 자연스러움.
	if (Cast<ASoldierUnit>(CurrentTarget))
	{
		const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}

	AttackTimer += DeltaTime;
	if (AttackTimer >= AttackInterval)
	{
		AttackTimer = 0.f;

		PlayAttackMontageOnAllMeshes();

		// ★ 실제 타격은 별도 함수로 분리. 보스처럼 광역 공격이 필요한 경우
		// 자식 클래스에서 이 함수만 오버라이드하면 됨.
		PerformAttack();
		// 데미지 적용 결과(파괴/비활성화 여부)는 다음 틱의 위 체크에서 자연스럽게 감지됨.
	}
}

void AMonsterBase::PerformAttack()
{
	if (!CurrentTarget)
	{
		return;
	}

	FDamageEvent DamageEvent;
	CurrentTarget->TakeDamage(AttackDamage, DamageEvent, nullptr, this);

	// ★ 확률적으로 타격 텍스트. 병사와 몬스터가 주고받는 느낌을 살림.
	if (FMath::FRand() <= HitTextChance)
	{
		AHitTextActor::SpawnHitText(this, HitTextClass,
			CurrentTarget->GetActorLocation() + HitTextOffset);
	}
}

void AMonsterBase::ApplySlow(float SlowRatio, float Duration)
{
	if (!SplineFollowComp)
	{
		return;
	}

	SplineFollowComp->SpeedMultiplier = FMath::Clamp(1.f - SlowRatio, 0.f, 1.f);

	GetWorldTimerManager().SetTimer(SlowTimerHandle, [this]()
		{
			if (SplineFollowComp)
			{
				SplineFollowComp->SpeedMultiplier = 1.f;
			}
		}, Duration, false);
}

void AMonsterBase::ApplyStun(float Duration)
{
	bIsStunned = true;

	GetWorldTimerManager().SetTimer(StunTimerHandle, [this]()
		{
			bIsStunned = false;
		}, Duration, false);
}

float AMonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (CurrentState == EMonsterState::Dead)
	{
		return 0.f;
	}

	CurrentHealth -= DamageAmount;
	UpdateHealthBar();

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return DamageAmount;
}

void AMonsterBase::UpdateHealthBar()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	if (UHealthBarWidget* Widget = Cast<UHealthBarWidget>(HealthBarWidgetComp->GetUserWidgetObject()))
	{
		const float Percent = (MaxHealth > 0.f) ? (CurrentHealth / MaxHealth) : 0.f;
		Widget->UpdateHealth(Percent);
	}

	RefreshHealthBarVisibility();
}

void AMonsterBase::RefreshHealthBarVisibility()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	// ★ "범위 안에 있음" + "풀피가 아님" 둘 다 만족해야 표시.
	// 평소엔 안 보이다가 맞은 애만 눈에 띄게 되어 화면이 훨씬 깔끔해짐.
	const bool bShouldShow = bPlayerInHealthBarRange && (CurrentHealth < MaxHealth) && CurrentState != EMonsterState::Dead;
	HealthBarWidgetComp->SetVisibility(bShouldShow);
}

void AMonsterBase::Die()
{
	CurrentState = EMonsterState::Dead;

	// ★ 죽는 순간 재료를 지급. 아이템을 바닥에 떨어뜨리지 않고 바로
	// 플레이어 보유량에 넣는 방식이라, 줍는 조작 없이도 재료가 모임.
	GiveDropMaterials();

	// 죽으면 더 이상 아무것도 감지/공격 못 하게 콜리전도 꺼둠 (몽타주 재생 중
	// 다른 몬스터/병사가 시체에 부딪히는 걸 방지)
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ★ 모든 메시(메인 + 추가)에 사망 몽타주를 재생하고, 그중 가장 긴 재생 시간을
	// 기준으로 파괴 타이밍을 잡음. 늑대는 3초짜리, 오크는 2초짜리 몽타주라면
	// 3초 뒤에 파괴해야 둘 다 자연스럽게 끝나기 때문.
	const float MontageDuration = PlayDeathMontageOnAllMeshes();
	if (MontageDuration > 0.f)
	{
		const float DestroyDelay = FMath::Max(0.f, MontageDuration - DeathMontageEndBuffer);
		GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &AMonsterBase::FinishDying, DestroyDelay, false);
		return;
	}

	// 몽타주가 없거나 재생 실패하면 그냥 바로 파괴
	FinishDying();
}

void AMonsterBase::PlayAttackMontageOnAllMeshes()
{
	// 메인 메시
	if (AttackMontage && MeshComp && MeshComp->GetAnimInstance())
	{
		MeshComp->GetAnimInstance()->Montage_Play(AttackMontage);
	}

	// ★ 추가 메시들 (울프 라이더의 늑대/오크처럼 메시가 여러 개인 경우)
	for (const FExtraMeshMontage& Extra : ExtraMeshMontages)
	{
		if (!Extra.AttackMontage)
		{
			continue;
		}

		if (USkeletalMeshComponent* ExtraMesh = FindMeshByName(Extra.MeshComponentName))
		{
			if (ExtraMesh->GetAnimInstance())
			{
				ExtraMesh->GetAnimInstance()->Montage_Play(Extra.AttackMontage);
			}
		}
	}
}

float AMonsterBase::PlayDeathMontageOnAllMeshes()
{
	float LongestDuration = 0.f;

	// 메인 메시
	if (DeathMontage && MeshComp && MeshComp->GetAnimInstance())
	{
		LongestDuration = FMath::Max(LongestDuration, MeshComp->GetAnimInstance()->Montage_Play(DeathMontage));
	}

	// 추가 메시들
	for (const FExtraMeshMontage& Extra : ExtraMeshMontages)
	{
		if (!Extra.DeathMontage)
		{
			continue;
		}

		if (USkeletalMeshComponent* ExtraMesh = FindMeshByName(Extra.MeshComponentName))
		{
			if (ExtraMesh->GetAnimInstance())
			{
				LongestDuration = FMath::Max(LongestDuration,
					ExtraMesh->GetAnimInstance()->Montage_Play(Extra.DeathMontage));
			}
		}
	}

	return LongestDuration;
}

bool AMonsterBase::IsAnyAttackMontagePlaying() const
{
	// 메인 메시
	if (AttackMontage && MeshComp && MeshComp->GetAnimInstance()
		&& MeshComp->GetAnimInstance()->Montage_IsPlaying(AttackMontage))
	{
		return true;
	}

	// 추가 메시들 중 하나라도 재생 중이면 true
	for (const FExtraMeshMontage& Extra : ExtraMeshMontages)
	{
		if (!Extra.AttackMontage)
		{
			continue;
		}

		if (USkeletalMeshComponent* ExtraMesh = FindMeshByName(Extra.MeshComponentName))
		{
			if (ExtraMesh->GetAnimInstance()
				&& ExtraMesh->GetAnimInstance()->Montage_IsPlaying(Extra.AttackMontage))
			{
				return true;
			}
		}
	}

	return false;
}

USkeletalMeshComponent* AMonsterBase::FindMeshByName(FName ComponentName) const
{
	TArray<USkeletalMeshComponent*> AllMeshes;
	GetComponents<USkeletalMeshComponent>(AllMeshes);

	for (USkeletalMeshComponent* Mesh : AllMeshes)
	{
		if (Mesh && Mesh->GetFName() == ComponentName)
		{
			return Mesh;
		}
	}
	return nullptr;
}

void AMonsterBase::GiveDropMaterials()
{
	if (!DropTable || DropRowNames.Num() == 0)
	{
		return; // 드랍 설정이 없는 몬스터
	}

	ACH6_Shop_SimulatorCharacter* Player =
		Cast<ACH6_Shop_SimulatorCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!Player || !Player->MaterialInventory)
	{
		return;
	}

	for (const FName& RowName : DropRowNames)
	{
		const FMonsterDropData* Row = DropTable->FindRow<FMonsterDropData>(RowName, TEXT("MonsterDrop"));
		if (!Row)
		{
			continue;
		}

		// 확률 판정. 철처럼 항상 주는 재료는 DropChance를 1로 두면 됨.
		if (FMath::FRand() > Row->DropChance)
		{
			continue;
		}

		const int32 Amount = FMath::RandRange(Row->MinAmount, Row->MaxAmount);
		Player->MaterialInventory->AddMaterial(Row->MaterialType, Amount);
	}
}

void AMonsterBase::FinishDying()
{
	// TODO: 사망 연출(파티클/사운드), 오브젝트 풀 반환 등 필요하면 여기 추가
	Destroy();
}