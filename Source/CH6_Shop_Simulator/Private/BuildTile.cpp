// Fill out your copyright notice in the Description page of Project Settings.

#include "BuildTile.h"
#include "BuildSelectionWidget.h"
#include "StructureManageWidget.h"
#include "Components/StaticMeshComponent.h"
#include "CH6_Shop_Simulator/CH6_Shop_SimulatorCharacter.h"
#include "DefenseWall.h"
#include "SoldierTower.h"
#include "BuildableLinkComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ABuildTile::ABuildTile()
{
	PrimaryActorTick.bCanEverTick = false;

	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	RootComponent = TileMesh;
}

void ABuildTile::BeginPlay()
{
	Super::BeginPlay();
}

void ABuildTile::TryOpenMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter)
{
	if (!InteractingCharacter)
	{
		return;
	}

	// ★ 이미 메뉴가 열려있으면 또 열지 않음. (중복으로 열리면 예전 위젯이 유령처럼
	// 남아서 입력이 꼬이고 아무것도 못 하게 되는 문제가 있었음)
	if (ActiveBuildWidget || ActiveManageWidget)
	{
		return;
	}

	// 이미 뭔가 지어져 있으면 판매/업그레이드 메뉴, 비어있으면 건설 메뉴.
	if (bIsOccupied)
	{
		OpenManageMenu(InteractingCharacter);
	}
	else
	{
		OpenBuildMenu(InteractingCharacter);
	}
}

void ABuildTile::OpenBuildMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter)
{
	if (!MenuWidgetClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InteractingCharacter->GetController());
	if (!PC)
	{
		return;
	}

	PendingInstigator = InteractingCharacter;

	ActiveBuildWidget = CreateWidget<UBuildSelectionWidget>(PC, MenuWidgetClass);
	if (ActiveBuildWidget)
	{
		ActiveBuildWidget->Setup(this, Options);
		ActiveBuildWidget->AddToViewport();
		FreezePlayerInput();

		// ★ 이제 번호키 대신 마우스 클릭으로 고르는 UI라서, 진짜 마우스 커서와
		// UI 클릭이 가능한 입력 모드가 필요함. (게임 이동도 같이 살려둬서, 혹시
		// 나중에 메뉴 열린 채로 다른 상호작용이 필요해져도 대응 가능하게 함 -
		// 실제 이동 자체는 bIsInteractionLocked가 이미 막고 있음)
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ActiveBuildWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void ABuildTile::OpenManageMenu(ACH6_Shop_SimulatorCharacter* InteractingCharacter)
{
	if (!ManageWidgetClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(InteractingCharacter->GetController());
	if (!PC)
	{
		return;
	}

	PendingInstigator = InteractingCharacter;

	const int32 RefundAmount = FMath::RoundToInt(PaidCost * SellRefundRatio);

	ActiveManageWidget = CreateWidget<UStructureManageWidget>(PC, ManageWidgetClass);
	if (ActiveManageWidget)
	{
		ActiveManageWidget->Setup(this, RefundAmount);
		ActiveManageWidget->AddToViewport();
		FreezePlayerInput();

		// 빌드 메뉴 때와 동일하게, 마우스 클릭 UI라서 커서와 UI 입력 모드가 필요함
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ActiveManageWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void ABuildTile::OnOptionSelected(int32 OptionIndex)
{
	if (!Options.IsValidIndex(OptionIndex) || !PendingInstigator)
	{
		return;
	}

	const FBuildOption& Chosen = Options[OptionIndex];

	// ★ 저장 데이터에서 복원하는 중에는 이미 지불한 건물이므로 골드를 건드리지 않음
	if (!bIsRestoringFromSave)
	{
		if (PendingInstigator->PlayerGold < Chosen.Cost)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("골드가 부족합니다!"));
			return; // 위젯은 계속 열어둠 (다른 항목을 고를 수 있게)
		}
	}

	if (!Chosen.StructureClass)
	{
		return;
	}

	if (!bIsRestoringFromSave)
	{
		PendingInstigator->PlayerGold -= Chosen.Cost;
	}

	// ★ SpawnLocationOverride가 지정되어 있으면 그 위치에, 아니면 이 타일 자신의 위치에 스폰.
	// (양옆 타일에서 조작하지만 실제 구조물은 가운데 길목에 생겨야 하는 경우를 위함)
	AActor* SpawnRef = SpawnLocationOverride ? SpawnLocationOverride.Get() : this;
	const FVector SpawnLoc = SpawnRef->GetActorLocation();
	const FRotator BaseSpawnRot = SpawnRef->GetActorRotation(); // 타일 순수 회전 (보정 전)

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* NewStructure = nullptr;

	if (Chosen.StructureClass->IsChildOf(ASoldierTower::StaticClass()))
	{
		// ★ 일반 SpawnActor()는 함수가 끝나기 전에 이미 BeginPlay()까지 실행해버려서,
		// 그 안에서 병사가 스폰되는 ASoldierTower는 FacingOffsetYaw를 나중에 설정해봐야
		// 이미 늦음. SpawnActorDeferred로 "액터는 만들되 BeginPlay는 아직 실행 안 함"
		// 상태로 받아서, 값을 먼저 설정한 다음 FinishSpawning으로 진짜 시작시킴.
		const FTransform SpawnTransform(BaseSpawnRot, SpawnLoc);
		AActor* DeferredActor = GetWorld()->SpawnActorDeferred<AActor>(
			Chosen.StructureClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (ASoldierTower* SpawnedTower = Cast<ASoldierTower>(DeferredActor))
		{
			SpawnedTower->FacingOffsetYaw = Chosen.SpawnRotationOffset.Yaw;
		}

		if (DeferredActor)
		{
			DeferredActor->FinishSpawning(SpawnTransform); // 여기서 비로소 BeginPlay 실행됨
		}

		NewStructure = DeferredActor;
	}
	else
	{
		const FRotator SpawnRot = BaseSpawnRot + Chosen.SpawnRotationOffset;
		NewStructure = GetWorld()->SpawnActor<AActor>(Chosen.StructureClass, SpawnLoc, SpawnRot, SpawnParams);
	}

	if (ADefenseWall* Wall = Cast<ADefenseWall>(NewStructure))
	{
		Wall->Activate();

		// ★ 전투로 파괴되면(Deactivate) 이 타일도, 짝꿍 타일도 다시 빈 상태로 되돌아가야 함.
		Wall->OnWallDeactivated.AddDynamic(this, &ABuildTile::HandleStructureDestroyed);
		if (LinkedTile)
		{
			Wall->OnWallDeactivated.AddDynamic(LinkedTile, &ABuildTile::HandleStructureDestroyed);
		}
	}

	BuiltStructure = NewStructure;
	PaidCost = Chosen.Cost;
	bIsOccupied = true;

	// ★ 이 구조물 자체를 클릭해도 관리 메뉴(판매/업그레이드)가 뜰 수 있도록,
	// "나는 이 타일 소속이야"를 알려주는 컴포넌트를 자동으로 붙여줌.
	if (NewStructure)
	{
		UBuildableLinkComponent* Link = NewObject<UBuildableLinkComponent>(NewStructure);
		if (Link)
		{
			Link->OwningTile = this;
			Link->RegisterComponent();
		}
	}

	// ★ 타일 위에 구조물이 지어졌으니, 타일 자신의 시각적 모습(메시)은 숨김
	// (bHideTileMeshWhenBuilt가 true인 타일만). 클릭 판정(콜리전)은 그대로 둬서
	// 판매 메뉴 상호작용은 계속 가능하게 함.
	if (TileMesh && bHideTileMeshWhenBuilt)
	{
		TileMesh->SetVisibility(false, true); // 두 번째 인자 true = 자식 컴포넌트(간판 등)까지 같이 숨김
	}

	// ★ 짝꿍 타일이 있으면 똑같은 구조물 참조를 공유하도록 상태 동기화.
	// (돈은 이미 여기서 한 번만 냈으므로 짝꿍 쪽은 추가 차감 없이 상태만 맞춰줌)
	if (LinkedTile)
	{
		LinkedTile->BuiltStructure = NewStructure;
		LinkedTile->PaidCost = Chosen.Cost;
		LinkedTile->bIsOccupied = true;

		if (LinkedTile->TileMesh && LinkedTile->bHideTileMeshWhenBuilt)
		{
			LinkedTile->TileMesh->SetVisibility(false, true); // 두 번째 인자 true = 자식 컴포넌트(간판 등)까지 같이 숨김
		}
	}

	if (ActiveBuildWidget)
	{
		ActiveBuildWidget->RemoveFromParent();
		ActiveBuildWidget = nullptr;
	}

	// ★ 저장 복원 중에는 UI/입력을 건드리지 않음.
	// (메뉴를 연 적이 없으므로 닫을 것도, 되돌릴 입력 상태도 없음.
	//  연출도 로드 시점에 한꺼번에 터지면 어색하므로 생략)
	if (bIsRestoringFromSave)
	{
		PendingInstigator = nullptr;
		return;
	}

	// ★ 건설 연출(파티클, 사운드)은 블루프린트에서 이 이벤트를 받아 처리
	OnStructureBuilt(SpawnLoc);

	if (APlayerController* PC = Cast<APlayerController>(PendingInstigator->GetController()))
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	UnfreezePlayerInput();
	PendingInstigator = nullptr;
}

void ABuildTile::OnSellSelected()
{
	if (!PendingInstigator)
	{
		return;
	}

	const int32 RefundAmount = FMath::RoundToInt(PaidCost * SellRefundRatio);
	PendingInstigator->PlayerGold += RefundAmount;

	// 파괴 전에 위치를 기억해둠 (연출을 그 자리에 띄우기 위함)
	const FVector SellLocation = BuiltStructure ? BuiltStructure->GetActorLocation() : GetActorLocation();

	if (BuiltStructure)
	{
		BuiltStructure->Destroy();
		BuiltStructure = nullptr;
	}

	// ★ 판매 연출(파티클, 사운드)은 블루프린트에서 이 이벤트를 받아 처리
	OnStructureSold(SellLocation);

	bIsOccupied = false;
	PaidCost = 0;

	// ★ 구조물이 사라졌으니 타일 자신의 시각적 모습을 다시 보여줌
	if (TileMesh)
	{
		TileMesh->SetVisibility(true, true); // 자식까지 같이 다시 보이게
	}

	// ★ 짝꿍 타일도 같은 구조물을 참조하고 있었으니, 파괴됐다는 걸 반영해서 다시 빈 상태로.
	if (LinkedTile)
	{
		LinkedTile->BuiltStructure = nullptr;
		LinkedTile->bIsOccupied = false;
		LinkedTile->PaidCost = 0;

		if (LinkedTile->TileMesh)
		{
			LinkedTile->TileMesh->SetVisibility(true, true); // 자식까지 같이 다시 보이게
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		FString::Printf(TEXT("판매 완료! %dG 환불되었습니다."), RefundAmount));

	if (ActiveManageWidget)
	{
		ActiveManageWidget->RemoveFromParent();
		ActiveManageWidget = nullptr;
	}

	if (PendingInstigator)
	{
		if (APlayerController* PC = Cast<APlayerController>(PendingInstigator->GetController()))
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}

	UnfreezePlayerInput();
	PendingInstigator = nullptr;
}

void ABuildTile::HandleStructureDestroyed()
{
	BuiltStructure = nullptr;
	bIsOccupied = false;
	PaidCost = 0;

	// ★ 여기도 마찬가지 (몬스터에게 바리케이드가 파괴된 경우 등)
	if (TileMesh)
	{
		TileMesh->SetVisibility(true, true); // 자식까지 같이 다시 보이게
	}
}

void ABuildTile::RestoreStructure(UClass* StructureClass, int32 InPaidCost)
{
	if (!StructureClass || bIsOccupied)
	{
		return;
	}

	// ★ 저장된 클래스와 일치하는 건설 옵션을 찾아서 기존 건설 흐름을 그대로 탐.
	// 스폰 위치/회전 보정, 병사 타워의 Deferred 스폰, 바리케이드 활성화 같은
	// 처리가 옵션마다 다르기 때문에, 직접 스폰하지 않고 옵션을 재사용함.
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		if (Options[i].StructureClass == StructureClass)
		{
			FoundIndex = i;
			break;
		}
	}

	if (FoundIndex == INDEX_NONE)
	{
		return; // 이 타일에서 지을 수 없는 구조물 (레벨이 바뀐 경우 등)
	}

	// 골드 차감 없이 건설만 수행
	bIsRestoringFromSave = true;
	PendingInstigator = Cast<ACH6_Shop_SimulatorCharacter>(
		UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	OnOptionSelected(FoundIndex);

	bIsRestoringFromSave = false;

	// 저장 당시의 비용으로 덮어씀 (환불 계산이 맞도록)
	PaidCost = InPaidCost;
}

void ABuildTile::OnMenuCancelled()
{
	// 빌드 메뉴든 관리 메뉴든, 둘 다 마우스 클릭 UI라서 닫을 때 게임 조작 모드로 되돌림
	const bool bWasMouseMenu = (ActiveBuildWidget != nullptr) || (ActiveManageWidget != nullptr);

	if (ActiveBuildWidget)
	{
		ActiveBuildWidget->RemoveFromParent();
		ActiveBuildWidget = nullptr;
	}
	if (ActiveManageWidget)
	{
		ActiveManageWidget->RemoveFromParent();
		ActiveManageWidget = nullptr;
	}

	if (bWasMouseMenu && PendingInstigator)
	{
		if (APlayerController* PC = Cast<APlayerController>(PendingInstigator->GetController()))
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}

	UnfreezePlayerInput();
	PendingInstigator = nullptr;
}

void ABuildTile::FreezePlayerInput()
{
	if (!PendingInstigator)
	{
		return;
	}

	// 이동/시점 회전 함수(Move/Look)에서 이 플래그를 체크해서 멈춤.
	// 상호작용(Interact)은 계속 살아있어서, 다시 누르면 OnMenuCancelled로 처리됨.
	PendingInstigator->bIsInteractionLocked = true;
	PendingInstigator->CurrentOpenBuildTile = this;

	// ★ InputMode는 이제 안 건드림. "두 번째 상호작용 = 메뉴 닫기"로 해결했기 때문에
	// 위젯이 포커스를 잃어도 문제가 안 생김. 위젯은 원래대로 자기가 직접
	// SetKeyboardFocus()로 포커스를 가져가서 1,2,3,Q 키를 받으면 충분함.
	// (InputMode를 GameOnly -> GameAndUI로 전환하는 과정에서 마우스 커서가
	// 잠깐 노출되는 부작용이 있었는데, 이제 이 전환 자체가 필요 없어짐)
}

void ABuildTile::UnfreezePlayerInput()
{
	if (!PendingInstigator)
	{
		return;
	}

	PendingInstigator->bIsInteractionLocked = false;
	PendingInstigator->CurrentOpenBuildTile = nullptr;
}