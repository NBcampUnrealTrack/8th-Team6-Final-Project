// Fill out your copyright notice in the Description page of Project Settings.

#include "TowerProjectile.h"
#include "MonsterBase.h"
#include "HitTextActor.h"
#include "DefenseWall.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"

ATowerProjectile::ATowerProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetMobility(EComponentMobility::Movable); // ProjectileMovementComponent로 옮기려면 반드시 Movable이어야 함

	// "Projectile" 콜리전 프리셋: Pawn(몬스터)에는 Overlap, WorldStatic(벽)에는 Block.
	// -> 몬스터를 맞히면 관통하지 않고 이벤트로 잡아서 우리가 직접 데미지+파괴 처리,
	//    벽에 맞으면 그냥 막혀서(Hit 이벤트) 조용히 사라지게 처리.
	MeshComp->SetCollisionProfileName(TEXT("Projectile"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->OnComponentHit.AddDynamic(this, &ATowerProjectile::OnHit);
	MeshComp->OnComponentBeginOverlap.AddDynamic(this, &ATowerProjectile::OnBeginOverlap);

	// ★ 콜리전 판정을 위해 메시 자산 자체는 필요하지만(모양이 있어야 충돌 계산이
	// 되니까), 화면에는 안 보이게 강제로 숨김. Visible 체크박스로 매번 신경 쓸
	// 필요 없이 코드가 항상 확실하게 숨겨줌 (콜리전에는 전혀 영향 없음 - 렌더링
	// 여부와 콜리전은 완전히 별개의 설정임).
	MeshComp->SetVisibility(false);

	// ★ 실제로 보이는 탄환 모델. MeshComp(콜리전 담당, bRotationFollowsVelocity로
	// 매 프레임 회전이 강제 갱신됨)의 자식으로 붙여서 위치/이동은 그대로 따라가되,
	// 이 컴포넌트 자신의 회전은 코드가 안 건드려서 자유롭게 보정 가능.
	ProjectileVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileVisualMesh"));
	ProjectileVisualMesh->SetupAttachment(MeshComp);
	ProjectileVisualMesh->SetMobility(EComponentMobility::Movable); // Movable인 MeshComp 밑에 붙으려면 이것도 Movable이어야 함
	ProjectileVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 콜리전은 MeshComp가 전담

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
	ProjectileMovementComp->SetUpdatedComponent(MeshComp); // 콜리전 있는 MeshComp를 확실하게 지정 (자동 추론에 안 맡김)
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bShouldBounce = false;
	ProjectileMovementComp->ProjectileGravityScale = 0.f; // 직선으로 날아가게 (중력 무시)
}

void ATowerProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);

	// 스폰 직후 자기를 쏜 타워(Owner)와 겹쳐서 바로 파괴되는 것을 방지.
	if (AActor* OwnerActor = GetOwner())
	{
		MeshComp->IgnoreActorWhenMoving(OwnerActor, true);
	}

	// ★ 타워 탄환은 아군 시설(바리케이드)을 무시하고 통과함 (아군 오사 방지).
	// 몬스터는 여전히 바리케이드에 물리적으로 막히는 것과는 별개 - 이건 탄환 전용 예외 처리.
	// 콜리전 채널을 직접 건드리는 대신, "이 액터는 무시해"를 개별 지정하는 방식이라
	// 채널 번호/이름을 몰라도 안전하게 동작함.
	for (TActorIterator<ADefenseWall> It(GetWorld()); It; ++It)
	{
		if (ADefenseWall* Wall = *It)
		{
			MeshComp->IgnoreActorWhenMoving(Wall, true);
		}
	}
}

void ATowerProjectile::InitProjectile(float InDamage, const FVector& InVelocity)
{
	Damage = InDamage;
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->Velocity = InVelocity;
	}
}

void ATowerProjectile::IgnoreActor(AActor* ActorToIgnore)
{
	if (MeshComp && ActorToIgnore)
	{
		MeshComp->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void ATowerProjectile::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return; // 자기를 쏜 타워는 무시
	}

	if (AMonsterBase* Monster = Cast<AMonsterBase>(OtherActor))
	{
		FDamageEvent DamageEvent;
		Monster->TakeDamage(Damage, DamageEvent, nullptr, this);

		TrySpawnHitText(Monster);

		Destroy();
	}
}

void ATowerProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == GetOwner())
	{
		return; // 자기를 쏜 타워는 무시
	}

	// 몬스터 콜리전 설정에 따라 Overlap이 아니라 Block(Hit)으로 잡히는 경우가 있어서
	// 여기서도 동일하게 데미지 처리. (안 그러면 탄환이 몬스터 몸에 박혀서 안 사라짐)
	if (AMonsterBase* Monster = Cast<AMonsterBase>(OtherActor))
	{
		FDamageEvent DamageEvent;
		Monster->TakeDamage(Damage, DamageEvent, nullptr, this);

		TrySpawnHitText(Monster);

		Destroy();
		return;
	}

	// 몬스터가 아닌 다른 것(벽 등)에 막혀서 멈춘 경우 -> 그냥 조용히 파괴
	Destroy();
}

void ATowerProjectile::TrySpawnHitText(AMonsterBase* Monster)
{
	if (!Monster)
	{
		return;
	}

	const bool bKilled = (Monster->CurrentState == EMonsterState::Dead);

	// ★ 처치했을 때 전용 문구가 지정돼 있으면 그걸 쓰고, 없으면 평소 문구를 씀.
	TSubclassOf<AHitTextActor> TextToUse = (bKilled && KillTextClass) ? KillTextClass : HitTextClass;
	if (!TextToUse)
	{
		return;
	}

	// 처치는 항상 띄우도록 설정할 수 있고, 평소 명중은 확률에 따름.
	// (궁수처럼 연사가 빠른 타워는 매번 뜨면 화면이 지저분해지므로)
	const bool bShouldShow = (bKilled && bAlwaysShowOnKill) || (FMath::FRand() <= HitTextChance);
	if (!bShouldShow)
	{
		return;
	}

	AHitTextActor::SpawnHitText(this, TextToUse, Monster->GetActorLocation() + HitTextOffset);
}