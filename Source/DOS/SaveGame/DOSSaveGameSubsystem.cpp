#include "DOSSaveGameSubsystem.h"
#include "DOSSaveGame.h"
#include "Timeline/DOSTimelineSubsystem.h"
#include "Case/DOSCaseSubsystem.h"
#include "Case/DOSEndingSubsystem.h"
#include "Case/DOSEndingRuleAsset.h"
#include "Settings/DOSLevelSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

namespace DOSSaveGameConstants
{
	static const FString SlotName = TEXT("DOSSave");
	static const int32 UserIndex = 0;
}

namespace
{
	// 레벨은 Project Settings -> Game -> **DOS Levels** 에서 지정한다.
	// 종전에는 여기에 L_TestRoom이 하드코딩되어 있어서 새 게임이 8월에 만든 테스트 방을 열었다.
	void OpenGameplayLevel(const UObject* WorldContext)
	{
		const UDOSLevelSettings* Settings = UDOSLevelSettings::Get();
		if (!Settings || Settings->GameplayLevel.IsNull())
		{
			UE_LOG(LogTemp, Error,
				TEXT("[DOS] Gameplay Level이 비어 있습니다 - Project Settings > Game > DOS Levels 에서 지정하세요."));
			return;
		}
		UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContext, Settings->GameplayLevel);
	}
}

void UDOSSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedTimelineSubsystem = Collection.InitializeDependency<UDOSTimelineSubsystem>();
	CachedCaseSubsystem = Collection.InitializeDependency<UDOSCaseSubsystem>();
	CachedEndingSubsystem = Collection.InitializeDependency<UDOSEndingSubsystem>();

	if (UDOSCaseSubsystem* CaseSubsystem = CachedCaseSubsystem.Get())
	{
		CaseSubsystem->OnActiveInterventionsChanged.AddDynamic(this, &UDOSSaveGameSubsystem::HandleActiveInterventionsChanged);
	}

	if (UDOSEndingSubsystem* EndingSubsystem = CachedEndingSubsystem.Get())
	{
		EndingSubsystem->OnEndingResolved.AddDynamic(this, &UDOSSaveGameSubsystem::HandleEndingResolved);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDOSSaveGameSubsystem::HandlePostLoadMap);

	if (UGameplayStatics::DoesSaveGameExist(DOSSaveGameConstants::SlotName, DOSSaveGameConstants::UserIndex))
	{
		SaveData = Cast<UDOSSaveGame>(UGameplayStatics::LoadGameFromSlot(DOSSaveGameConstants::SlotName, DOSSaveGameConstants::UserIndex));
		bHasSaveOnDisk = SaveData != nullptr;
	}

	if (!SaveData)
	{
		SaveData = Cast<UDOSSaveGame>(UGameplayStatics::CreateSaveGameObject(UDOSSaveGame::StaticClass()));
	}
}

void UDOSSaveGameSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

bool UDOSSaveGameSubsystem::HasSaveGame() const
{
	return bHasSaveOnDisk && SaveData && SaveData->bHasActiveCheckpoint;
}

bool UDOSSaveGameSubsystem::HasTrueEndingUnlocked() const
{
	return SaveData && SaveData->bTrueEndingUnlocked;
}

void UDOSSaveGameSubsystem::StartNewGame()
{
	bPendingContinueRestore = false;

	if (UDOSCaseSubsystem* CaseSubsystem = CachedCaseSubsystem.Get())
	{
		CaseSubsystem->RestoreState(TArray<FName>(), TArray<FName>());
	}

	if (UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get())
	{
		Timeline->RestoreState(0.f, 1);
	}

	if (SaveData)
	{
		SaveData->bHasActiveCheckpoint = false;
		SaveData->TimelineTimeSeconds = 0.f;
		SaveData->CurrentRound = 1;
		SaveData->ActiveInterventions.Reset();
		SaveData->CollectedClues.Reset();
		SaveToDisk();
	}

	OpenGameplayLevel(this);
}

void UDOSSaveGameSubsystem::ContinueGame()
{
	if (!HasSaveGame())
	{
		StartNewGame();
		return;
	}

	bPendingContinueRestore = true;
	OpenGameplayLevel(this);
}

void UDOSSaveGameSubsystem::HandleActiveInterventionsChanged()
{
	if (!SaveData || !CachedTimelineSubsystem.IsValid() || !CachedCaseSubsystem.IsValid())
	{
		return;
	}

	const UDOSTimelineSubsystem* Timeline = CachedTimelineSubsystem.Get();
	const UDOSCaseSubsystem* Case = CachedCaseSubsystem.Get();

	SaveData->bHasActiveCheckpoint = true;
	SaveData->TimelineTimeSeconds = Timeline->GetCurrentTime();
	SaveData->CurrentRound = Timeline->GetCurrentRound();
	SaveData->ActiveInterventions = Case->GetActiveInterventions().Array();
	SaveData->CollectedClues = Case->GetCollectedClues().Array();

	SaveToDisk();
}

void UDOSSaveGameSubsystem::HandleEndingResolved(FName EndingId)
{
	if (!SaveData)
	{
		return;
	}

	bool bShouldSave = false;

	// The case just concluded - whatever checkpoint was mid-case is no longer something
	// Continue should resume into, true ending or not.
	if (SaveData->bHasActiveCheckpoint)
	{
		SaveData->bHasActiveCheckpoint = false;
		SaveData->TimelineTimeSeconds = 0.f;
		SaveData->CurrentRound = 1;
		SaveData->ActiveInterventions.Reset();
		SaveData->CollectedClues.Reset();
		bShouldSave = true;
	}

	if (!SaveData->bTrueEndingUnlocked && CachedEndingSubsystem.IsValid())
	{
		const UDOSEndingRuleAsset* EndingRules = CachedEndingSubsystem->EndingRules;
		const FDOSEndingRule* MatchedRule = EndingRules ? EndingRules->Rules.FindByPredicate([EndingId](const FDOSEndingRule& Rule)
		{
			return Rule.EndingId == EndingId;
		}) : nullptr;

		if (MatchedRule && MatchedRule->bIsTrueEnding)
		{
			SaveData->bTrueEndingUnlocked = true;
			bShouldSave = true;
		}
	}

	if (bShouldSave)
	{
		SaveToDisk();
	}
}

void UDOSSaveGameSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!bPendingContinueRestore)
	{
		return;
	}

	bPendingContinueRestore = false;
	ApplyCheckpointToGameplaySystems();
}

void UDOSSaveGameSubsystem::ApplyCheckpointToGameplaySystems()
{
	if (!SaveData || !CachedCaseSubsystem.IsValid() || !CachedTimelineSubsystem.IsValid())
	{
		return;
	}

	// Case first: interventions must be in place before the timeline broadcasts its restored
	// time, so NPC schedules resolve overrides correctly before they resample position.
	CachedCaseSubsystem->RestoreState(SaveData->ActiveInterventions, SaveData->CollectedClues);
	CachedTimelineSubsystem->RestoreState(SaveData->TimelineTimeSeconds, SaveData->CurrentRound);
}

void UDOSSaveGameSubsystem::SaveToDisk()
{
	if (!SaveData)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveData, DOSSaveGameConstants::SlotName, DOSSaveGameConstants::UserIndex);
	bHasSaveOnDisk = true;
}
