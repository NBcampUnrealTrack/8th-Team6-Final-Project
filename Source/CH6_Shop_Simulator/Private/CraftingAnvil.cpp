// Fill out your copyright notice in the Description page of Project Settings.


#include "CraftingAnvil.h"
#include "ProductBase.h"
#include "CraftingWidget.h"
#include "CH6_Shop_SimulatorCharacter.h"
#include "MaterialInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"

ACraftingAnvil::ACraftingAnvil()
{
	PrimaryActorTick.bCanEverTick = false;

	AnvilMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnvilMesh"));
	RootComponent = AnvilMesh;

	// ★ 클릭 판정을 받아야 하므로 콜리전을 켜둠. 스켈레탈 메시는 Physics Asset이
	// 있어야 라인트레이스에 잡히므로, 메시 에셋에 그게 지정되어 있는지 확인할 것.
	// (없으면 스켈레탈 메시 에디터에서 Create Physics Asset으로 만들면 됨)
	AnvilMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AnvilMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 제작된 상품이 나타날 위치 (대장장이 옆 허공에서 떨어지게)
	ProductSpawnSlot = CreateDefaultSubobject<USceneComponent>(TEXT("ProductSpawnSlot"));
	ProductSpawnSlot->SetupAttachment(RootComponent);
	ProductSpawnSlot->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
}

void ACraftingAnvil::BeginPlay()
{
	Super::BeginPlay();
}

void ACraftingAnvil::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 제작 타이머가 남지 않도록 정리
	GetWorldTimerManager().ClearTimer(CraftTimerHandle);

	Super::EndPlay(EndPlayReason);
}

TArray<FName> ACraftingAnvil::GetRecipeRowNames() const
{
	if (!RecipeDataTable)
	{
		return TArray<FName>();
	}
	return RecipeDataTable->GetRowNames();
}

void ACraftingAnvil::OpenCraftingMenu(ACH6_Shop_SimulatorCharacter* InUser)
{
	if (!InUser || !CraftingWidgetClass || !RecipeDataTable)
	{
		return;
	}

	CurrentUser = InUser;

	APlayerController* PC = Cast<APlayerController>(InUser->GetController());
	if (!PC)
	{
		return;
	}

	ActiveWidget = CreateWidget<UCraftingWidget>(PC, CraftingWidgetClass);
	if (!ActiveWidget)
	{
		return;
	}

	ActiveWidget->AddToViewport();
	ActiveWidget->SetupForAnvil(this, InUser);

	// ★ 메뉴를 보는 동안에는 이동과 시점을 멈추고 커서를 띄움.
	// (빌드 타일 메뉴와 같은 방식)
	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeGameAndUI());

	InUser->bIsInteractionLocked = true;
}

void ACraftingAnvil::CloseCraftingMenu()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}

	if (CurrentUser)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentUser->GetController()))
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}

		CurrentUser->bIsInteractionLocked = false;
		CurrentUser = nullptr;
	}
}

bool ACraftingAnvil::CraftProduct(FName RecipeRowName, int32 Count)
{
	if (!RecipeDataTable || !CurrentUser || !CurrentUser->MaterialInventory || Count <= 0)
	{
		return false;
	}

	// ★ 이미 제작 중이면 새 주문을 받지 않음.
	// (여러 주문이 섞이면 어느 상품이 나올지 관리가 복잡해짐)
	if (IsCrafting())
	{
		return false;
	}

	const FCraftRecipeData* Recipe = RecipeDataTable->FindRow<FCraftRecipeData>(RecipeRowName, TEXT("Craft"));
	if (!Recipe || !Recipe->ProductClass)
	{
		return false;
	}

	// ★ 수량만큼 재료를 곱해서 한 번에 검사하고 소모함.
	// 하나씩 처리하면 중간에 부족해질 때 처리가 지저분해짐.
	TArray<FMaterialCost> TotalCosts;
	for (const FMaterialCost& Cost : Recipe->Costs)
	{
		FMaterialCost Scaled = Cost;
		Scaled.Amount = Cost.Amount * Count;
		TotalCosts.Add(Scaled);
	}

	if (!CurrentUser->MaterialInventory->ConsumeMaterials(TotalCosts))
	{
		return false; // 재료 부족
	}

	// ★ 재료는 주문 시점에 전부 소모하고, 상품만 시간차로 하나씩 나옴.
	// 한꺼번에 50개가 쏟아지면 물리 연산 부담에 바닥도 뒤덮이기 때문.
	// UI를 닫아도 제작은 계속 진행됨.
	CraftingRecipeName = RecipeRowName;
	RemainingCraftCount = Count;

	GetWorldTimerManager().SetTimer(CraftTimerHandle, this,
		&ACraftingAnvil::ForgeNextItem, FMath::Max(0.1f, CraftTimePerItem), true);

	OnProductCrafted(Count);
	return true;
}

void ACraftingAnvil::ForgeNextItem()
{
	if (RemainingCraftCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(CraftTimerHandle);
		return;
	}

	SpawnProduct(CraftingRecipeName);
	OnSingleItemForged();

	RemainingCraftCount--;

	if (RemainingCraftCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(CraftTimerHandle);
		CraftingRecipeName = NAME_None;

		OnCraftingFinished();
	}
}

void ACraftingAnvil::SpawnProduct(FName ProductRowName)
{
	if (!ProductDataTable || !ProductSpawnSlot || !RecipeDataTable)
	{
		return;
	}

	const FCraftRecipeData* Recipe = RecipeDataTable->FindRow<FCraftRecipeData>(ProductRowName, TEXT("Spawn"));
	if (!Recipe || !Recipe->ProductClass)
	{
		return;
	}

	// ★ 여러 개를 동시에 만들 때 같은 자리에 겹쳐 스폰되지 않도록
	// 위치를 조금씩 흩어뜨림. 물리가 켜져 있어서 알아서 굴러떨어짐.
	const FVector RandomOffset(
		FMath::RandRange(-30.f, 30.f),
		FMath::RandRange(-30.f, 30.f),
		FMath::RandRange(0.f, 40.f));

	const FVector SpawnLoc = ProductSpawnSlot->GetComponentLocation() + RandomOffset;
	const FRotator SpawnRot = ProductSpawnSlot->GetComponentRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProductBase* NewProduct = GetWorld()->SpawnActor<AProductBase>(
		Recipe->ProductClass, SpawnLoc, SpawnRot, SpawnParams);

	if (!NewProduct)
	{
		return;
	}

	// 상품이 자기 정보를 읽어오도록 테이블과 행 이름을 주입
	NewProduct->ProductDataTable = ProductDataTable;
	NewProduct->ProductRowName = ProductRowName;
	NewProduct->InitializeProductInfo();

	// 물리를 켜서 아래로 떨어뜨림
	if (UPrimitiveComponent* BoxComp = Cast<UPrimitiveComponent>(NewProduct->GetRootComponent()))
	{
		BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoxComp->SetSimulatePhysics(true);
	}
}