#include "DOSRoomAnchor.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#define LOCTEXT_NAMESPACE "DOSRoomAnchor"

ADOSRoomAnchor::ADOSRoomAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

#if WITH_EDITORONLY_DATA
	// 방향을 눈으로 볼 수 있어야 한다 - 사람들이 어느 쪽을 보고 서는지가 앵커 회전으로 정해진다.
	if (UArrowComponent* Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow")))
	{
		Arrow->SetupAttachment(GetRootComponent());
		Arrow->ArrowSize = 1.5f;
		Arrow->bIsEditorOnly = true;
	}
	if (UBillboardComponent* Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		Sprite->SetupAttachment(GetRootComponent());
		Sprite->bIsEditorOnly = true;
	}
#endif
}

FVector ADOSRoomAnchor::GetSlotLocation(int32 Slot) const
{
	const FVector Origin = GetActorLocation();
	if (Slot <= 0 || SlotCount <= 1 || SlotRadius <= KINDA_SMALL_NUMBER)
	{
		return Origin;
	}

	// 슬롯 0을 중앙에 두고 나머지를 원 위에 고르게 벌린다.
	const int32 RingCount = FMath::Max(1, SlotCount - 1);
	const int32 RingIndex = (Slot - 1) % RingCount;
	const float Angle = (2.f * PI * RingIndex) / RingCount;

	const FVector Offset(FMath::Cos(Angle) * SlotRadius, FMath::Sin(Angle) * SlotRadius, 0.f);
	return Origin + GetActorRotation().RotateVector(Offset);
}

void ADOSRoomAnchor::GatherAnchors(const UWorld* World, TMap<FName, ADOSRoomAnchor*>& OutByRoomId)
{
	OutByRoomId.Reset();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADOSRoomAnchor> It(World); It; ++It)
	{
		ADOSRoomAnchor* Anchor = *It;
		if (!Anchor || Anchor->RoomId.IsNone())
		{
			continue;
		}
		if (ADOSRoomAnchor** Existing = OutByRoomId.Find(Anchor->RoomId))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[DOS] RoomId '%s' 앵커가 둘 이상입니다 ('%s'를 쓰고 '%s'는 무시합니다)."),
				*Anchor->RoomId.ToString(), *(*Existing)->GetName(), *Anchor->GetName());
			continue;
		}
		OutByRoomId.Add(Anchor->RoomId, Anchor);
	}
}

#if WITH_EDITOR
void ADOSRoomAnchor::CheckForErrors()
{
	Super::CheckForErrors();

	// RoomId가 비어 있으면 임포터가 조용히 건너뛴다. 맵 체크에서 미리 잡아준다.
	if (RoomId.IsNone())
	{
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("NoRoomId", "DOS 방 앵커에 RoomId가 비어 있습니다 - 스케줄 임포트에서 무시됩니다.")));
	}
	if (FloorId.IsNone())
	{
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("NoFloorId", "DOS 방 앵커에 FloorId가 비어 있습니다 (basement / f1 / f2 / attic).")));
	}
}
#endif

#undef LOCTEXT_NAMESPACE
