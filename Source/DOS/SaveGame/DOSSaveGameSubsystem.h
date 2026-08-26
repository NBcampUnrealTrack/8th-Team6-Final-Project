// Owns the single save slot: loads it at startup, keeps an autosave checkpoint current as the
// player intervenes in the case, and permanently records the first time the true ending is
// reached. The main menu reads HasSaveGame()/HasTrueEndingUnlocked() to configure its buttons
// and background; New Game/Continue both route through here rather than calling OpenLevel
// directly so the checkpoint stays consistent.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DOSSaveGameSubsystem.generated.h"

class UDOSSaveGame;
class UDOSTimelineSubsystem;
class UDOSCaseSubsystem;
class UDOSEndingSubsystem;

UCLASS()
class DOS_API UDOSSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// True only while there's an in-progress case checkpoint to resume - false once an ending
	// has been reached (nothing left to continue into) or before the first checkpoint is made.
	UFUNCTION(BlueprintPure, Category = "DOS|Save")
	bool HasSaveGame() const;

	UFUNCTION(BlueprintPure, Category = "DOS|Save")
	bool HasTrueEndingUnlocked() const;

	// Resets the in-progress checkpoint (the true-ending unlock flag is untouched) and opens
	// the gameplay level fresh.
	UFUNCTION(BlueprintCallable, Category = "DOS|Save")
	void StartNewGame();

	// Opens the gameplay level and, once it finishes loading, restores the last saved
	// checkpoint into DOSTimelineSubsystem/DOSCaseSubsystem. No-ops into StartNewGame() if
	// there is no save yet.
	UFUNCTION(BlueprintCallable, Category = "DOS|Save")
	void ContinueGame();

private:
	UFUNCTION()
	void HandleActiveInterventionsChanged();

	UFUNCTION()
	void HandleEndingResolved(FName EndingId);

	void HandlePostLoadMap(UWorld* LoadedWorld);
	void ApplyCheckpointToGameplaySystems();
	void SaveToDisk();

	UPROPERTY(Transient)
	TObjectPtr<UDOSSaveGame> SaveData;

	bool bHasSaveOnDisk = false;
	bool bPendingContinueRestore = false;

	TWeakObjectPtr<UDOSTimelineSubsystem> CachedTimelineSubsystem;
	TWeakObjectPtr<UDOSCaseSubsystem> CachedCaseSubsystem;
	TWeakObjectPtr<UDOSEndingSubsystem> CachedEndingSubsystem;
};
