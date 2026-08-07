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

	// True if the sampled time fell before the first waypoint or after the last one, i.e.
	// the state was held rather than interpolated.
	UPROPERTY(BlueprintReadOnly, Category = "DOS|Character")
	bool bIsClamped = false;
};
