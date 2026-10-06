// Copyright Epic Games, Inc. All Rights Reserved.

#include "CH6_Shop_SimulatorCharacter.h"
#include "CH6_Shop_SimulatorProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PhysicsEngine/PhysicsHandleComponent.h" 
#include "ProductBase.h"
#include "DisplayBase.h"
#include "CustomerAI.h"
#include "CounterBase.h"
#include "CraftingAnvil.h"
#include "BuildTile.h"
#include "Opensignactor.h"
#include "TrainPawn.h"
#include "BaseCore.h"
#include "BuildableLinkComponent.h"
#include "ShopPlacementComponent.h"
#include "MaterialInventoryComponent.h"
#include "InteractableComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraShakeBase.h"
#include "ShopBuildStation.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ACH6_Shop_SimulatorCharacter

ACH6_Shop_SimulatorCharacter::ACH6_Shop_SimulatorCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	PhysicsHandleComp = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandleComp"));

	ShopPlacementComp = CreateDefaultSubobject<UShopPlacementComponent>(TEXT("ShopPlacementComp"));

	// 재료 보유량 관리 (몬스터 드랍 수집, 제작 시 소모)
	MaterialInventory = CreateDefaultSubobject<UMaterialInventoryComponent>(TEXT("MaterialInventory"));
}

void ACH6_Shop_SimulatorCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ★ 스턴 시 시야각을 부드럽게 좁혔다가 되돌림. 값을 바로 바꾸면
	// 화면이 툭 튀어서 어색하므로 목표값을 향해 서서히 따라가게 함.
	// (TargetFOV는 ApplyStun/EndStun에서만 설정되므로, 스턴을 한 번도 안 겪었으면
	//  아래 조건이 false라 아무 일도 안 일어남)
	if (TargetFOV > 0.f && FirstPersonCameraComponent
		&& !FMath::IsNearlyEqual(FirstPersonCameraComponent->FieldOfView, TargetFOV, 0.1f))
	{
		const float NewFOV = FMath::FInterpTo(
			FirstPersonCameraComponent->FieldOfView, TargetFOV, DeltaTime, StunFOVInterpSpeed);
		FirstPersonCameraComponent->SetFieldOfView(NewFOV);
	}

	// ★ 상호작용 대상 탐색. 매 프레임 할 필요는 없어서 일정 주기로만 확인함
	// (0.1초면 체감상 즉각적이면서 연산은 훨씬 적음).
	InteractionCheckTimer += DeltaTime;
	if (InteractionCheckTimer >= InteractionCheckInterval)
	{
		InteractionCheckTimer = 0.f;
		UpdateInteractionTarget();
	}

	// ★ [내 손바닥 전구가 켜져 있고, 자석 부품이 정상 장착되어 있다면?]
	if (bIsHoldingProduct && PhysicsHandleComp)
	{
		// 1. 카메라의 현재 세계 좌표 위치 추출 [🌐]
		FVector CameraLocation = FirstPersonCameraComponent->GetComponentLocation();

		// 2. 카메라 전면 방향 벡터로 150cm 앞마당 최종 목적지 좌표 계산 🔒 [🌐]
		FVector TargetLocation = CameraLocation + (FirstPersonCameraComponent->GetForwardVector() * 100.f);

		// 3. 물리 핸들에게 "이 상자를 매 프레임 이 위치로 부드럽게 끌고 와라!" 지시 쾅! [🌐]
		PhysicsHandleComp->SetTargetLocation(TargetLocation);
	}
}
//////////////////////////////////////////////////////////////////////////// Input

void ACH6_Shop_SimulatorCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ACH6_Shop_SimulatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACH6_Shop_SimulatorCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACH6_Shop_SimulatorCharacter::Look);

		EnhancedInputComponent->BindAction(InteractAction_LeftClick, ETriggerEvent::Started, this, &ACH6_Shop_SimulatorCharacter::OnInteractPressed);
		EnhancedInputComponent->BindAction(CancelAction_RightClick, ETriggerEvent::Started, this, &ACH6_Shop_SimulatorCharacter::OnCancelPressed);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

// ──────────────────────────────────────────────────────────────────────
// ⭐ [교통정리 타워]: 메인 상호작용 함수는 이제 조건문 딱 하나로만 심플하게 움직입니다!
// ──────────────────────────────────────────────────────────────────────
void ACH6_Shop_SimulatorCharacter::OnInteractPressed()
{
	// ★ 배치 모드(진열대 구매 후 미리보기) 중이면, 다른 어떤 상호작용보다도
	// 우선해서 지금 위치에 확정 배치. (메뉴 잠금 체크보다도 먼저 - 배치 중엔
	// bIsInteractionLocked랑 무관하게 항상 좌클릭=확정으로 동작해야 하니까)
	if (ShopPlacementComp && ShopPlacementComp->IsPlacing())
	{
		ShopPlacementComp->ConfirmPlacement();
		return;
	}

	// ★ 메뉴가 열려있는 상태에서 다시 상호작용 키를 누르면 -> 뭘 보고 있든 상관없이
	// 그냥 지금 열린 메뉴를 닫는 것으로 처리. (두 번째 클릭이 애매하게 씹히거나
	// 새 메뉴가 겹쳐 열리는 문제를 원천적으로 방지)
	if (bIsInteractionLocked)
	{
		if (CurrentOpenBuildTile)
		{
			CurrentOpenBuildTile->OnMenuCancelled();
		}
		return;
	}
	if (TryMountTrain()) { return; }
	if (TryOpenSign()) { return; }
	if (TryOpenShopBuildStation()) { return; }
	// 0. 최우선: 지금 보고 있는 게 설치 타일이면, 손에 뭘 들고 있든 상관없이 그것부터 처리하고 끝냄.
	//    (다른 상호작용 - 진열, 줍기, 계산 - 과 겹쳐서 같이 실행될 일이 없도록 여기서 끊음)
	if (TryOpenBuildTile()) { return; }

	// 상황 A: 이미 손에 상자가 들려있다면? ──► 오직 "진열대 배치" 기능만 발동!
	if (bIsHoldingProduct) { TryPlaceProductOnDisplay(); }

	// 상황 B: 빈손 상태라면? ──► "손님 계산", "진열대 재배치", "물건 줍기"를 차례대로 찔러봅니다!
	else
	{
		// 1. 눈앞에 계산 대기 중인 손님이 있으면 결제가 최우선.
		//    (손님 뒤에 진열대가 겹쳐 보일 때 엉뚱하게 진열대를 집어드는 걸 방지)
		if (TryCheckoutCustomer()) { return; }

		// 2. 손님이 없으면 진열대 자체를 재배치 시도
		if (TryPickupExistingDisplay()) { return; }

		// 3. 그것도 아니면 바닥의 물건 줍기를 시도합니다.
		if (!bIsHoldingProduct)
		{
			TryPickupProductFromWorld();
		}
	}
}

// ──────────────────────────────────────────────────────────────────────
// 📦 [비서 1]: 진열대에 상자 배치
// ──────────────────────────────────────────────────────────────────────
void ACH6_Shop_SimulatorCharacter::TryPlaceProductOnDisplay()
{
	if (!PhysicsHandleComp) return;

	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLoc = StartLoc + (FirstPersonCameraComponent->GetForwardVector() * 250.f);

	FHitResult DisplayHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	UPrimitiveComponent* GrabbedComponent = PhysicsHandleComp->GetGrabbedComponent();
	if (GrabbedComponent && GrabbedComponent->GetOwner())
	{
		QueryParams.AddIgnoredActor(GrabbedComponent->GetOwner());
	}

	if (GetWorld()->LineTraceSingleByChannel(DisplayHit, StartLoc, EndLoc, ECC_Visibility, QueryParams))
	{
		ADisplayBase* HitDisplay = Cast<ADisplayBase>(DisplayHit.GetActor());
		if (HitDisplay)
		{
			if (GrabbedComponent)
			{
				AProductBase* ProductActor = Cast<AProductBase>(GrabbedComponent->GetOwner());
				if (ProductActor)
				{
					// 방화벽: 타입 일치 및 빈자리 검사
					if (HitDisplay->AcceptableProductType != ProductActor->ProductType) return;
					if (!HitDisplay->IsSlotEmpty()) return;

					// 1. 캐릭터와 물건 간 충돌 무시 제거
					this->MoveIgnoreActorRemove(ProductActor);

					// 2. 물리를 끄고 콜리전을 완전히 차단하여 플레이어가 다시는 주워갈 수 없게 잠금 처리! 🔒
					GrabbedComponent->SetSimulatePhysics(false);
					GrabbedComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

					// 3. 물리 핸들 자석 해제
					PhysicsHandleComp->ReleaseComponent();
					bIsHoldingProduct = false;

					// 4. 강제 텔레포트 후 본드 결착
					FVector SlotLoc = HitDisplay->DisplaySlot->GetComponentLocation();
					FRotator SlotRot = HitDisplay->DisplaySlot->GetComponentRotation();
					ProductActor->SetActorLocationAndRotation(SlotLoc, SlotRot, false, nullptr, ETeleportType::TeleportPhysics);
					ProductActor->AttachToComponent(HitDisplay->DisplaySlot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

					// 5. 장부 기록 및 손님들에게 알림 오픈
					HitDisplay->OccupiedProduct = ProductActor;
					HitDisplay->NotifyCustomersProductAvailable();

					GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("상품이 안전하게 진열 및 잠금 처리되었습니다!"));
					return;
				}
			}
		}
	}
	// ──────────────────────────────────────────────────────────────────────
	// 🔥 [2단계 분기]: 눈앞에 진열대가 없거나 허공/바닥/벽을 보고 클릭했다면?
	// ──► 시뮬레이터답게 그 자리에 상자를 '툭' 떨어뜨려 자유로운 배치를 보장합니다! 📦
	// ──────────────────────────────────────────────────────────────────────
	if (GrabbedComponent && GrabbedComponent->GetOwner())
	{
		// 1. 내 몸뚱이와 상자 간의 충돌 무시를 제거하여 다시 발로 차거나 비벼지게 만듭니다.
		this->MoveIgnoreActorRemove(GrabbedComponent->GetOwner());

		// 2. 물리 핸들 자석 전원을 띡 꺼버립니다. (중력에 의해 바닥으로 툭 떨어짐)
		PhysicsHandleComp->ReleaseComponent();
		bIsHoldingProduct = false;

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("[LOG] 상자를 일반 바닥에 떨어뜨렸습니다."));
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("[LOG] 진열대를 정확히 조준하여 배치해 주세요!"));
}

// ──────────────────────────────────────────────────────────────────────
// 🖐️ [비서 2]: 바닥에 떨어진 물건 상자를 물리 핸들로 집어 올리는 독자 영역
// ──────────────────────────────────────────────────────────────────────
void ACH6_Shop_SimulatorCharacter::TryPickupProductFromWorld()
{
	if (!PhysicsHandleComp) return;

	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLoc = StartLoc + (FirstPersonCameraComponent->GetForwardVector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, QueryParams))
	{
		// ★ 대장장이(제작대)를 클릭하면 제작 UI를 엶.
		// 예전에는 클릭 즉시 확률로 랜덤 아이템이 나왔는데, 플레이어가
		// 원하는 걸 골라 만들 수 있도록 선택 제작 방식으로 바꿈.
		if (ACraftingAnvil* HitAnvil = Cast<ACraftingAnvil>(HitResult.GetActor()))
		{
			HitAnvil->OpenCraftingMenu(this);
			return;
		}

		// 레이저가 조준한 대상이 상자(`AProductBase`)가 맞다면?
		AProductBase* HitProduct = Cast<AProductBase>(HitResult.GetActor());
		if (HitProduct && HitResult.GetComponent())
		{
			// 만약 마우스로 조준한 이 상자가 이미 손님이 들고 가는 물건(`bIsOwnedByCustomer == true`)이라면?
			// 플레이어가 아무리 마우스를 광클해도 줍기를 즉시 거부하고 철저히 씹어버립니다! ✅
			if (HitProduct->bIsOwnedByCustomer)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("손님이 구매 중인 상품은 빼앗을 수 없습니다!"));
				return; // 줍기 성공 코드를 타지 못하게 즉시 함수 탈출! 쾅!
			}

			for (TActorIterator<ADisplayBase> It(GetWorld()); It; ++It)
			{
				ADisplayBase* CheckingDisplay = *It;
				if (CheckingDisplay && CheckingDisplay->OccupiedProduct == HitProduct)
				{
					CheckingDisplay->OccupiedProduct = nullptr; // "이 진열대 이제 빈방입니다!" 🧹
					HitProduct->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // 부모 자식 끊기
					break; // 찾았으니 조사 루프 조기 탈출
				}
			}
			// ──────────────────────────────────────────────────────────────────────

			// ★ [가장 중요]: 진열 시점에 물리를 껐으므로, 다시 집어 올릴 때는 
			// 물리 시뮬레이션 상태 여부와 상관없이 무조건 물리와 충돌 설정을 원래대로 켜줍니다! ✅
			HitResult.GetComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			HitResult.GetComponent()->SetSimulatePhysics(true);
			// 잡을 때 내 캐릭터 몸뚱이와 부딪혀서 우주로 날아가는 것 방지
			this->MoveIgnoreActorAdd(HitProduct);

			// 물리 핸들로 결착 🔗
			PhysicsHandleComp->GrabComponentAtLocationWithRotation(
				HitResult.GetComponent(),
				NAME_None,
				HitResult.Location,
				FRotator::ZeroRotator
			);

			bIsHoldingProduct = true;
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("바닥의 상품 상자를 줍기 성공했습니다!"));

		}
	}
}

// ──────────────────────────────────────────────────────────────────────
// 💸 [비서 3]: 계산대 대기열 맨 앞 손님을 클릭해 돈 버는 정산 독자 영역
// ──────────────────────────────────────────────────────────────────────
bool ACH6_Shop_SimulatorCharacter::TryCheckoutCustomer()
{
	FVector StartLoc = FirstPersonCameraComponent->GetComponentLocation();
	FVector EndLoc = StartLoc + (FirstPersonCameraComponent->GetForwardVector() * 250.f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, QueryParams))
	{
		ACustomerAI* TargetCustomer = Cast<ACustomerAI>(HitResult.GetActor());

		if (TargetCustomer && TargetCustomer->TargetDisplay)
		{
			ACounterBase* Counter = nullptr;
			for (TActorIterator<ACounterBase> It(GetWorld()); It; ++It)
			{
				Counter = *It;
				if (Counter) break;
			}

			// 계산대가 존재하고, 대기자가 있으며, 맨 앞줄(0번) 손님이 맞다면 정산 시작!
			if (Counter && Counter->CustomerQueue.Num() > 0 && Counter->CustomerQueue[0] == TargetCustomer)
			{
				if (TargetCustomer->CurrentState == ECustomerState::WaitingForCheckout)
				{
					TArray<AActor*> AttachedActors;
					TargetCustomer->GetAttachedActors(AttachedActors);

					AProductBase* HeldProduct = nullptr;
					for (AActor* AttachedActor : AttachedActors)
					{
						HeldProduct = Cast<AProductBase>(AttachedActor);
						if (HeldProduct) break;
					}

					if (HeldProduct)
					{
						// 기본가 추출 및 진열대 버프 반영
						int32 BaseItemPrice = HeldProduct->Price;
						float DisplayBuff = 1.0f;
						if (TargetCustomer->TargetDisplay)
						{
							DisplayBuff = TargetCustomer->TargetDisplay->PriceMultiplier;
						}

						int32 FinalEarnedGold = FMath::FloorToInt(BaseItemPrice * DisplayBuff);

						// 플레이어의 골드 누적
						PlayerGold += FinalEarnedGold;

						// 🔥 [개조형 정석 등급별 스탯 연산소 ⚔️🛡️]
						// ──────────────────────────────────────────────────────────────────────
						// A 분기: 무기를 판 경우 ──► 공격력 창고 적립
						if (HeldProduct->ProductType == EProductType::Weapon)
						{
							// 1) 고정치 더하기인 경우 (일반, 레어) ──► BaseAttackPower에 누적! (+)
							if (HeldProduct->MyBonusType == EStatBonusType::Flat)
							{
								BaseAttackPower += FMath::FloorToInt(HeldProduct->MyBonusValue);
							}
							// 2) 퍼센트 배율인 경우 (에픽 3%, 전설 10%) ──► AttackMultiplier에 계수 누적! (%)
							else if (HeldProduct->MyBonusType == EStatBonusType::Percentage)
							{
								// 3% 면 0.03f를 기존 배율에 더해줍니다. (예: 1.0f + 0.03f = 1.03f)
								AttackMultiplier += (HeldProduct->MyBonusValue / 100.f);
							}
						}
						// B 분기: 방어구를 판 경우 ──► 성벽 체력 창고 적립
						else if (HeldProduct->ProductType == EProductType::Armor)
						{
							// 1) 고정치 더하기인 경우 ──► BaseMaxHealth에 누적! (+)
							if (HeldProduct->MyBonusType == EStatBonusType::Flat)
							{
								BaseMaxHealth += FMath::FloorToInt(HeldProduct->MyBonusValue);
							}
							// 2) 퍼센트 배율인 경우 ──► HealthMultiplier에 계수 누적! (%)
							else if (HeldProduct->MyBonusType == EStatBonusType::Percentage)
							{
								HealthMultiplier += (HeldProduct->MyBonusValue / 100.f);
							}
						}

						// ⭐️ [가장 중요]: 수치 적립이 끝났으니, 최종 공식 함수를 발동해 결괏값을 갱신합니다!
						UpdateFinalStats();

						// 로그 출력 (FinalAttackPower 수치를 보여줍니다)
						GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Cyan,
							FString::Printf(TEXT("⚔️ [%s 판매 완료] 고정공격력:%d | 배율:x%.2f ──► 💥 최종 공격력: %d !!"),
								*HeldProduct->ItemName, BaseAttackPower, AttackMultiplier, FinalAttackPower));
						// ──────────────────────────────────────────────────────────────────────
						// 화면 정산 성공 로그 출력
						GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Green,
							FString::Printf(TEXT("💵 [결제 완료] %s 판매! (버프: x%.1f) ──► 최종 +%d 골드 입금!"),
								*HeldProduct->ItemName, PlayerGold));

						// 계산이 끝났으니 퇴장 지점으로 나가게 함
						// (좌표는 손님이 스폰될 때 레벨 마커에서 이미 뽑아둔 값을 사용)
						TargetCustomer->HeadToExit();

						Counter->PopQueue();
						return true; // 계산 성공
					}
				}

			}
			else
			{
				// 아직 줄서기 지정 칸에 걸어가는 중인 뒷사람을 클릭했을 때의 안전 방화벽 메시지
				GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("해당 손님은 아직 자신의 대기선 자리에 도달하지 않았습니다!"));
			}
		}
	}
	return false;
}


void ACH6_Shop_SimulatorCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ★ 시작 스탯도 한 번 반영해둠. 안 그러면 첫 판매가 일어나기 전까지
	// 기차와 코어가 자기 기본값으로 남아있음.
	UpdateFinalStats();
}

void ACH6_Shop_SimulatorCharacter::UpdateFinalStats()
{
	// 스텟 계산 공식: [고정 공격력 * 퍼센트 배율 = 최종 공격력] (소수점 버림)
	FinalAttackPower = FMath::FloorToInt(BaseAttackPower * AttackMultiplier);

	// 최대 체력도 똑같이 연산!
	FinalMaxHealth = FMath::FloorToInt(BaseMaxHealth * HealthMultiplier);

	// ★ 계산만 하고 끝내면 아무 데도 반영이 안 되므로, 실제로 쓰는 쪽에
	// 값을 밀어넣어 줌. (바리케이드는 지어질 때 직접 읽어가므로 제외)
	ApplyStatsToWorld();
}

void ACH6_Shop_SimulatorCharacter::ApplyStatsToWorld()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 무기를 팔면 올라간 공격력을 기차 터렛에 반영
	for (TActorIterator<ATrainPawn> It(World); It; ++It)
	{
		if (ATrainPawn* Train = *It)
		{
			Train->Damage = static_cast<float>(FinalAttackPower);
		}
	}

	// 방어구를 팔면 올라간 체력을 베이스 코어에 반영
	for (TActorIterator<ABaseCore> It(World); It; ++It)
	{
		ABaseCore* Core = *It;
		if (!Core)
		{
			continue;
		}

		// ★ 최대 체력이 늘어난 만큼 현재 체력도 같이 늘려줌.
		// 안 그러면 최대치만 오르고 실제로는 반쯤 깎인 상태가 됨.
		const int32 OldMax = Core->MaxHealth;
		const int32 Increase = FinalMaxHealth - OldMax;

		Core->MaxHealth = FinalMaxHealth;
		if (Increase > 0)
		{
			Core->CurrentHealth = FMath::Min(Core->CurrentHealth + Increase, Core->MaxHealth);
		}

		// HUD 체력바가 새 최대치를 반영하도록 알림
		Core->NotifyHealthChanged();
	}
}

bool ACH6_Shop_SimulatorCharacter::TryOpenBuildTile()
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 300.f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		AActor* HitActor = Hit.GetActor();

		if (ABuildTile* Tile = Cast<ABuildTile>(HitActor))
		{
			Tile->TryOpenMenu(this);
			return true;
		}

		// ★ 타일 자체가 아니라, 그 위에 지어진 구조물(타워/병사타워/바리케이드 등)을
		// 직접 클릭한 경우 - 링크 컴포넌트로 소속 타일을 찾아서 그쪽 메뉴를 띄움
		if (UBuildableLinkComponent* Link = HitActor ? HitActor->FindComponentByClass<UBuildableLinkComponent>() : nullptr)
		{
			if (Link->OwningTile)
			{
				Link->OwningTile->TryOpenMenu(this);
				return true;
			}
		}
	}
	return false;
}

void ACH6_Shop_SimulatorCharacter::Move(const FInputActionValue& Value)
{
	if (bIsInteractionLocked) { return; } // 메뉴 열려있으면 무시 (스턴은 이동 속도로만 제약)

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ACH6_Shop_SimulatorCharacter::Look(const FInputActionValue& Value)
{
	if (bIsInteractionLocked) { return; } // 메뉴 열려있으면 무시 (스턴은 이동 속도로만 제약)

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

bool ACH6_Shop_SimulatorCharacter::TryOpenSign()
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 300.f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		if (AOpenSignActor* Sign = Cast<AOpenSignActor>(Hit.GetActor()))
		{
			Sign->TryOpen();
			return true;
		}
	}
	return false;
}

bool ACH6_Shop_SimulatorCharacter::TryMountTrain()
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 300.f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		if (ATrainPawn* Train = Cast<ATrainPawn>(Hit.GetActor()))
		{
			// ★ 터렛 부분을 맞혔을 때만 탑승 허용 (몸체 옆면 클릭은 무시).
			// 실제로 보이는 메시는 TurretVisualMesh로 옮겼기 때문에 그쪽도 같이
			// 확인해야 함 (TurretHeadComp는 이제 메시 없는 조준 피벗 역할).
			UPrimitiveComponent* HitComp = Hit.GetComponent();
			if (HitComp == Train->TurretHeadComp || HitComp == Train->TurretVisualMesh)
			{
				Train->MountPawn(this);
				return true;
			}
		}
	}
	return false;
}

void ACH6_Shop_SimulatorCharacter::PlayRejectionShake()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (RejectionCameraShake && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(RejectionCameraShake);
		}
	}
}

void ACH6_Shop_SimulatorCharacter::UpdateInteractionTarget()
{
	UInteractableComponent* FoundInteractable = nullptr;

	// 배치 모드 중이거나 메뉴가 열려있으면 상호작용 대상 탐색 자체를 건너뜀
	// (그 상황에선 다른 걸 클릭할 수 없으니 하이라이트를 띄울 이유도 없음)
	// ★ IsPlayerControlled 체크: 기차에 탑승하면 조종권이 기차로 넘어가서
	// 이 캐릭터는 조종 대상이 아니게 됨. 그런데 Tick은 계속 돌기 때문에,
	// 이 체크가 없으면 탑승 중에도 계속 앞을 스캔해서 자기가 타고 있는
	// 터렛을 상호작용 대상으로 인식하고 UI가 계속 떠있는 문제가 있었음.
	const bool bIsBusy = bIsInteractionLocked
		|| !IsPlayerControlled()
		|| (ShopPlacementComp && ShopPlacementComp->IsPlacing());

	if (!bIsBusy && FirstPersonCameraComponent)
	{
		const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
		const FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * InteractionDistance);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				UInteractableComponent* Interactable = HitActor->FindComponentByClass<UInteractableComponent>();

				// 컴포넌트가 있고, 지금 실제로 상호작용 가능한 상태여야 표시
				// (예: 상품이 올라간 진열대는 CanInteract에서 false를 반환하게 만들 수 있음)
				// ★ IsValidHitComponent: 기차처럼 "특정 부위(터렛)를 정확히 봐야만"
				// 인식되게 설정한 경우, 다른 부위를 봤을 땐 여기서 걸러짐.
				if (Interactable && Interactable->CanInteract()
					&& Interactable->IsValidHitComponent(Hit.GetComponent()))
				{
					FoundInteractable = Interactable;
				}
			}
		}
	}

	// ★ 대상이 바뀌었을 때만 실제로 켜고 끔. 같은 걸 계속 바라보고 있으면
	// 아무 작업도 안 일어남.
	if (CurrentInteractable != FoundInteractable)
	{
		if (CurrentInteractable)
		{
			CurrentInteractable->SetHighlighted(false);
		}

		CurrentInteractable = FoundInteractable;

		if (CurrentInteractable)
		{
			CurrentInteractable->SetHighlighted(true);
		}
	}
}

void ACH6_Shop_SimulatorCharacter::ApplyStun(float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}

	// 처음 걸릴 때만 원래 값을 기억 (연속으로 맞아도 누적되지 않게)
	if (!bIsStunned)
	{
		if (FirstPersonCameraComponent)
		{
			OriginalFOV = FirstPersonCameraComponent->FieldOfView;
		}
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			OriginalWalkSpeed = MoveComp->MaxWalkSpeed;
			MoveComp->MaxWalkSpeed = OriginalWalkSpeed * StunSpeedMultiplier;

			// ★ 진단용 로그 (원인 파악 후 지워도 됨)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Magenta,
				FString::Printf(TEXT("[스턴] 속도 %.0f -> %.0f (배율 %.2f)"),
					OriginalWalkSpeed, MoveComp->MaxWalkSpeed, StunSpeedMultiplier));
		}
	}

	bIsStunned = true;
	TargetFOV = OriginalFOV - StunFOVReduction; // Tick에서 부드럽게 따라감

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	GetWorldTimerManager().SetTimer(StunTimerHandle, this,
		&ACH6_Shop_SimulatorCharacter::EndStun, Duration, false);
}

void ACH6_Shop_SimulatorCharacter::EndStun()
{
	bIsStunned = false;
	TargetFOV = OriginalFOV;

	// 이동 속도 복원
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (OriginalWalkSpeed > 0.f)
		{
			MoveComp->MaxWalkSpeed = OriginalWalkSpeed;
		}
	}
}

void ACH6_Shop_SimulatorCharacter::OnCancelPressed()
{
	// ★ 취소 기능은 이제 안 씀 (구매는 확정이라 되돌릴 필요가 없어졌음).
	// 대신 배치 모드 중일 때 우클릭으로 90도씩 회전시키는 용도로 재활용함.
	if (ShopPlacementComp && ShopPlacementComp->IsPlacing())
	{
		ShopPlacementComp->RotateGhost90();
	}
}

bool ACH6_Shop_SimulatorCharacter::TryOpenShopBuildStation()
{
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 300.f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		if (AShopBuildStation* Station = Cast<AShopBuildStation>(Hit.GetActor()))
		{
			Station->TryOpen(this);
			return true;
		}
	}
	return false;
}

bool ACH6_Shop_SimulatorCharacter::TryPickupExistingDisplay()
{
	// 이미 배치 모드 중이면 중복으로 못 건드리게 막음
	if (ShopPlacementComp && ShopPlacementComp->IsPlacing())
	{
		return false;
	}

	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 300.f);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
	{
		if (ADisplayBase* Display = Cast<ADisplayBase>(Hit.GetActor()))
		{
			// ★ 상품이 진열되어 있으면 재배치 못 하게 막음. 상품(별도 액터)은
			// 진열대를 옮겨도 같이 안 따라오기 때문에, 그대로 두면 상품이 원래
			// 자리에 남아서 겹침 검사에 계속 걸려 영원히 재배치가 안 되는 버그가
			// 생김. "먼저 상품을 정리해야 옮길 수 있다"는 규칙이 더 직관적이고,
			// 손님이 그 상품으로 걸어가는 도중에 진열대가 움직여버리는 어색한
			// 상황도 원천적으로 방지됨.
			if (!Display->IsSlotEmpty())
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("상품이 진열된 진열대는 옮길 수 없습니다. 먼저 상품을 정리해주세요."));
				PlayRejectionShake();
				return false;
			}

			// 이미 고스트 상태(다른 경로로 배치 중)인 진열대는 건드리지 않음
			if (!Display->IsInGhostMode() && ShopPlacementComp)
			{
				ShopPlacementComp->BeginPlacementMode(Display);
				return true;
			}
		}
	}
	return false;
}

bool ACH6_Shop_SimulatorCharacter::BuyAndPlaceDisplay(TSubclassOf<ADisplayBase> DisplayClass, int32 Cost, FVector SpawnLocation)
{
	if (!DisplayClass || !GetWorld())
	{
		return false;
	}

	if (PlayerGold < Cost)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("골드가 부족합니다!"));
		PlayRejectionShake();
		return false;
	}

	PlayerGold -= Cost;

	// ★ 배치 모드 없이 그냥 지정된 구역에 평범하게 스폰만 함 (돈은 이미 확정).
	// 이후 이 진열대를 클릭하면(TryPickupExistingDisplay) 그때 배치 모드로 들어감.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<ADisplayBase>(DisplayClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

	return true;
}