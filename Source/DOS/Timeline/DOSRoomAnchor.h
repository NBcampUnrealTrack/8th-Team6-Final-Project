// 방 하나의 기준점. 레벨에 방마다 하나씩 놓고 RoomId를 지정해두면, 스케줄 임포터가 이걸
// 훑어서 웨이포인트의 Location/Rotation을 채운다.
//
// 왜 데이터 테이블이 아니라 레벨 액터인가: 좌표를 손으로 적어두면 레벨을 조금만 옮겨도
// 조용히 어긋난다. 앵커는 방과 같이 움직이므로 어긋날 일이 없고, 블록아웃을 다시 잡아도
// 앵커만 따라 옮기면 스케줄을 다시 임포트하는 것으로 끝난다.
//
// RoomId는 웹 프로토타입(src/data/rooms.ts)의 id를 그대로 쓴다:
//   지하 - storage / boiler / cell / laundry
//   1층  - lobby / parlor / chapel / kitchen / maidroom / dining / ballroom
//   2층  - hall / gallery / bed1 / bed2 / bed3 / master / dressing / bath / study
//   다락 - atticstore

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DOSRoomAnchor.generated.h"

UCLASS()
class DOS_API ADOSRoomAnchor : public AActor
{
	GENERATED_BODY()

public:
	ADOSRoomAnchor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Room")
	FName RoomId;

	// basement / f1 / f2 / attic
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Room")
	FName FloorId;

	// 한 방에 여러 사람이 동시에 서 있는 장면이 많다(저녁식사 5명, 구출 6명). 전부 같은 점에
	// 세우면 겹쳐서 한 사람으로 보이므로, 앵커를 중심으로 원형으로 벌려 세운다.
	// 인물마다 고정된 슬롯을 쓰므로 같은 인물은 늘 같은 자리에 선다 - 웹 프로토타입의
	// occupantAnchorTile과 같은 방식이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Room", meta = (ClampMin = "0.0"))
	float SlotRadius = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Room", meta = (ClampMin = "1"))
	int32 SlotCount = 7;

	// 슬롯 0은 앵커 자리 그대로, 1부터는 원 위에 벌려 놓는다.
	UFUNCTION(BlueprintPure, Category = "DOS|Room")
	FVector GetSlotLocation(int32 Slot) const;

	// 그 방에 서면 어느 쪽을 보고 있어야 하는지. 앵커의 회전을 그대로 쓴다 - 방을 돌려놓으면
	// 사람들도 같이 돌아본다.
	UFUNCTION(BlueprintPure, Category = "DOS|Room")
	FRotator GetSlotRotation() const { return GetActorRotation(); }

	// 월드에 놓인 앵커를 RoomId로 모아 준다. 같은 RoomId가 둘 이상이면 첫 번째만 쓰고
	// 경고를 남긴다 - 조용히 하나를 버리면 어느 쪽이 쓰였는지 알 수 없다.
	static void GatherAnchors(const UWorld* World, TMap<FName, ADOSRoomAnchor*>& OutByRoomId);

#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif
};
