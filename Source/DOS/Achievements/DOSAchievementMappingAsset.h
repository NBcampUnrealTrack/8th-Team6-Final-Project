// Maps internal, design-facing achievement keys (used by code/data - ending IDs, clue IDs,
// custom event keys) to the real Steamworks achievement API ID. Left mostly empty until the
// Steamworks app is registered; unmapped keys are simply skipped at unlock time.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DOSAchievementMappingAsset.generated.h"

USTRUCT(BlueprintType)
struct FDOSAchievementMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Achievement")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DOS|Achievement")
	FString SteamAchievementId;
};

UCLASS(BlueprintType)
class DOS_API UDOSAchievementMappingAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Achievement")
	TArray<FDOSAchievementMapping> Mappings;
};
