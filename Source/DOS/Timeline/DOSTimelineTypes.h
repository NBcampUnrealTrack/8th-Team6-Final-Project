// Shared data types for the master timeline / NPC schedule system (System 1).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DOSTimelineTypes.generated.h"

UENUM(BlueprintType)
enum class EDOSTimelinePlayState : uint8
{
	Paused,
	Playing,
	Rewinding
};

// A single scripted point in a character's schedule: where they are, what they're doing,
// and when. FloorId/RoomId are free-form tags rather than references into a specific level,
// so schedules can be authored before the house layout is finalized.
USTRUCT(BlueprintType)
struct FDOSWaypoint
{
	GENERATED_BODY()

	// Seconds elapsed since the case timeline started (0 - 1800 for a 30 minute case).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	float TimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	FName FloorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	FName RoomId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	FRotator Rotation = FRotator::ZeroRotator;

	// Semantic action from this waypoint onward, e.g. Action.Idle, Action.Sit, Action.Hide.
	// Actual animation/effect playback for this tag is handled in Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint", meta = (Categories = "Action"))
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	TSoftObjectPtr<UAnimMontage> ActionMontage;

	// 이 박자를 **직접 보면** 그 지식을 얻는다. 비워두면 목격 대상이 아니다.
	//
	// 오브젝트는 그 행위 자체를 본 뒤에만 열린다 - 세바스찬이 칼을 **집어드는** 20:10을
	// 봐야 식칼을 밀 수 있고, 들고 걷는 20:25를 본 것으로는 열리지 않는다.
	// 짝이 되는 것은 UDOSInterventionComponent::RequiredWitnessId.
	// FName이므로 파이썬 임포터로 넣어도 안전하다(텍스트가 아니다).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Waypoint")
	FName WitnessId = NAME_None;
};

// Result of sampling a character's schedule at a given timeline time.
USTRUCT(BlueprintType)
struct FDOSCharacterState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FName FloorId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FName RoomId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FGameplayTag ActionTag;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	TSoftObjectPtr<UAnimMontage> ActionMontage;

	// 지금 재생 중인 박자가 목격 대상이면 그 id. ADOSScheduledCharacter가 이걸 보고
	// 플레이어에게 실제로 보이는지 판정한다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FName WitnessId = NAME_None;

	// 지금 향해야 하는 웨이포인트의 **보간하지 않은** 자리.
	//
	// 위의 Location은 웨이포인트 사이를 직선 보간한 값이라, 순간이동 배치
	// (bApplyTransformToOwner)에는 맞지만 **걸어가는 목표로는 쓸 수 없다.**
	// 걸음(175)이 보간 목표의 이동 속도(초당 수십)보다 빨라서 즉시 따라잡아 멈추고,
	// 목표가 조금 앞서면 또 한 걸음 - 결과가 「걷다 멈추다」 반복이고 속도가 대부분 0이라
	// 걷는 애니메이션도 안 나온다. 회전도 「멈춤=웨이포인트 방향 / 걷기=이동 방향」을
	// 왕복한다.
	//
	// 그래서 NavMesh로 걸을 때는 이 값을 목표로 쓴다. 웨이포인트가 바뀔 때만 변하므로
	// 경로 요청이 한 번씩만 나가고 걸음이 끊기지 않는다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FVector GoalLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	FRotator GoalRotation = FRotator::ZeroRotator;

	// 그 목표에 **닿아야 하는 시각**. 걸음 속도를 남은 시간에 맞추는 데 쓴다 -
	// 웹은 「타일 하나 = 1초」라 항상 제때 닿았는데, 3D는 거리가 다르므로 속도로 맞춘다.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	float GoalTimeSeconds = 0.f;

	// True if the sampled time fell before the first waypoint or after the last one, i.e.
	// the state was held rather than interpolated.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	bool bIsClamped = false;
};
