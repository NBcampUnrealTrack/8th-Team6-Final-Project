// Wraps the platform online subsystem's achievements interface (Steamworks via
// OnlineSubsystemSteam once registered) behind a generic string-key API, and hooks itself into
// System 2's ending/clue events. Safe to run with no online subsystem present (e.g. no Steam
// client) - unlock calls are logged and skipped rather than failing.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DOSAchievementSubsystem.generated.h"

class UDOSAchievementMappingAsset;
struct FDOSAchievementMapping;

UCLASS()
class DOS_API UDOSAchievementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DOS|Achievement")
	TObjectPtr<UDOSAchievementMappingAsset> MappingAsset;

	// Unlocks the achievement mapped to Key. No-ops (with a log message) if Key has no
	// mapping yet, or no online subsystem/achievements interface/user is available.
	UFUNCTION(BlueprintCallable, Category = "DOS|Achievement")
	void UnlockAchievement(FName Key);

private:
	UFUNCTION()
	void HandleEndingResolved(FName EndingId);

	UFUNCTION()
	void HandleClueDiscovered(FName ClueId);

	const FDOSAchievementMapping* FindMapping(FName Key) const;
};
