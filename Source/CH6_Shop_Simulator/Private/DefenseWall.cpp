// Fill out your copyright notice in the Description page of Project Settings.

#include "DefenseWall.h"
#include "CH6_Shop_Simulator/CH6_Shop_SimulatorCharacter.h"
#include "SplinePathManager.h"
#include "Components/SplineComponent.h"
#include "Components/WidgetComponent.h"
#include "HealthBarWidget.h"
#include "Kismet/GameplayStatics.h"

ADefenseWall::ADefenseWall()
{
	// ★ HealthBarWidgetComp는 여기서 CreateDefaultSubobject로 안 만듦!
	// 블루프린트에서 직접 이 이름 그대로 위젯 컴포넌트를 추가하면 자동으로
	// 이 변수에 연결됨 (1인칭 캐릭터의 Mesh1P와 같은 방식). 이렇게 하면
	// "생성자 시점엔 루트가 아직 없어서 새 컴포넌트가 루트를 차지해버리는" 문제
	// 자체가 원천적으로 안 생김 - 블루프린트가 원하는 메시 밑에 자유롭게 붙일 수 있음.
}

void ADefenseWall::BeginPlay()
{
	Super::BeginPlay();

	// ★ 이 시점엔 블루프린트 쪽 메시 설정이 다 끝나서 RootComponent가 확실히 존재함.
	// 생성자에서 미리 붙이지 않고 여기서 붙이는 이유는 클래스 주석 참고.
	// ★ 블루프린트에서 HealthBarWidgetComp를 추가해주셨다면, 여기서 필요한
	// 초기 설정(화면 항상 정면으로 보이게, 초기 숨김)을 적용.
	// Draw Size는 여기서 강제로 지정하지 않음 - 블루프린트 디테일 패널에서
	// 설정한 값이 게임 시작할 때마다 덮어써지면 안 되니까.
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
		HealthBarWidgetComp->SetVisibility(false); // 처음엔 숨김 (Activate 전이니까)
	}

	// 여기서는 스탯을 읽지 않음! (레벨 시작 시점 값이라 이미 낡았을 수 있음)
	// 대신 레벨 시작 시엔 "존재하지만 안 보이고 안 막는" 초기 상태로만 세팅.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	bIsActive = false;

	// 활성/비활성 여부와 무관하게, "이 킬존 지점에 내가 있다"는 것 자체는
	// 레벨 시작 시 미리 등록해둠. (몬스터는 나중에 bIsActive를 조회해서 실제로 막혔는지 판단)
	if (KillZoneSplineActor)
	{
		KillZoneSpline = KillZoneSplineActor->FindComponentByClass<USplineComponent>();
	}

	if (KillZoneSpline)
	{
		// ★ 숫자를 손으로 입력하지 않고, 이 액터가 놓인 위치에서 스플라인 위 가장 가까운
		// 지점을 찾아 그 거리값을 자동으로 계산. 디자이너는 그냥 눈으로 보고 배치만 하면 됨.
		const float InputKey = KillZoneSpline->FindInputKeyClosestToWorldLocation(GetActorLocation());
		KillZoneDistance = KillZoneSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);

		if (USplinePathManager* PathManager = GetWorld()->GetSubsystem<USplinePathManager>())
		{
			PathManager->RegisterKillZone(KillZoneSpline, KillZoneDistance, this);
		}
	}
}

void ADefenseWall::Activate()
{
	if (bIsActive)
	{
		return; // 이미 설치되어 있으면 중복 처리 방지
	}

	// ★ 활성화되는 "지금 이 순간"의 최신 스탯을 읽어옴 (BeginPlay가 아니라 여기서!)
	ACH6_Shop_SimulatorCharacter* PlayerChar =
		Cast<ACH6_Shop_SimulatorCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (PlayerChar)
	{
		MaxHealth = PlayerChar->FinalMaxHealth / 2;
		CurrentHealth = MaxHealth;
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	bIsActive = true;
	// 등록은 이미 BeginPlay에서 끝났음. 몬스터는 이 bIsActive 값을 직접 조회하므로
	// 여기서 매니저에 별도로 알릴 필요는 없음 (RegisterKillZone은 딱 1번, 위치 정보만 등록).

	UpdateHealthBar();

	// ★ Tick이 꺼져있는 액터라서, 거리 체크는 Timer로 대신함. 활성화됐을 때만 필요하니
	// 여기서 시작하고, Deactivate()에서 멈춤.
	GetWorldTimerManager().SetTimer(HealthBarVisibilityTimerHandle, this,
		&ADefenseWall::CheckHealthBarPlayerDistance, 0.3f, true);

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
		FString::Printf(TEXT("바리케이드 설치 완료! 체력: %d"), MaxHealth));
}

void ADefenseWall::Deactivate()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	bIsActive = false;
	// 몬스터가 CurrentTarget->bIsActive를 직접 조회하므로, 여기서 값만 false로 바꿔주면
	// TickAttacking 쪽에서 다음 틱에 자동으로 "막힘 해제"를 감지함.

	GetWorldTimerManager().ClearTimer(HealthBarVisibilityTimerHandle);
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(false);
	}

	// ★ 나를 지은 BuildTile에게도 "이제 없어졌다"고 알림 -> 타일이 다시 빈 상태로 돌아가서
	// 재설치 메뉴가 뜨게 됨. (안 하면 타일은 계속 "지어진 상태"로 착각해서 판매 메뉴만 뜸)
	OnWallDeactivated.Broadcast();
}

float ADefenseWall::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsActive)
	{
		return 0.f; // 비활성 상태(설치 안 됨)면 애초에 맞을 수 없음
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - FMath::FloorToInt(DamageAmount), 0, MaxHealth);
	UpdateHealthBar();

	if (CurrentHealth <= 0)
	{
		// 부모(ADefenseObjectBase)처럼 Destroy() 해버리면 다음 웨이브 때 재설치가 불가능해짐.
		// 대신 Deactivate()로 되돌려서 "다시 설치 가능한 빈 타일" 상태로 재사용.
		Deactivate();
	}

	return DamageAmount;
}

void ADefenseWall::UpdateHealthBar()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	if (UHealthBarWidget* Widget = Cast<UHealthBarWidget>(HealthBarWidgetComp->GetUserWidgetObject()))
	{
		const float Percent = (MaxHealth > 0) ? (static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)) : 0.f;
		Widget->UpdateHealth(Percent);
	}

	RefreshHealthBarVisibility();
}

void ADefenseWall::RefreshHealthBarVisibility()
{
	if (!HealthBarWidgetComp)
	{
		return;
	}

	const bool bShouldShow = bIsActive && bPlayerInHealthBarRange && (CurrentHealth < MaxHealth);
	HealthBarWidgetComp->SetVisibility(bShouldShow);
}

void ADefenseWall::CheckHealthBarPlayerDistance()
{
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