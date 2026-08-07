#include "DOSAchievementSubsystem.h"
#include "DOSAchievementMappingAsset.h"
#include "Case/DOSCaseSubsystem.h"
#include "Case/DOSEndingSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineAchievementsInterface.h"

void UDOSAchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UDOSEndingSubsystem* EndingSubsystem = Collection.InitializeDependency<UDOSEndingSubsystem>())
	{
		EndingSubsystem->OnEndingResolved.AddDynamic(this, &UDOSAchievementSubsystem::HandleEndingResolved);
	}

	if (UDOSCaseSubsystem* CaseSubsystem = Collection.InitializeDependency<UDOSCaseSubsystem>())
	{
		CaseSubsystem->OnClueDiscovered.AddDynamic(this, &UDOSAchievementSubsystem::HandleClueDiscovered);
	}
}

const FDOSAchievementMapping* UDOSAchievementSubsystem::FindMapping(FName Key) const
{
	if (!MappingAsset)
	{
		return nullptr;
	}

	return MappingAsset->Mappings.FindByPredicate([Key](const FDOSAchievementMapping& Mapping)
	{
		return Mapping.Key == Key;
	});
}

void UDOSAchievementSubsystem::UnlockAchievement(FName Key)
{
	const FDOSAchievementMapping* Mapping = FindMapping(Key);
	if (!Mapping || Mapping->SteamAchievementId.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("DOSAchievementSubsystem: no Steam mapping for key '%s', skipping unlock."), *Key.ToString());
		return;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = OnlineSub ? OnlineSub->GetAchievementsInterface() : nullptr;
	if (!Achievements.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DOSAchievementSubsystem: no achievements interface available, skipping unlock for '%s'."), *Key.ToString());
		return;
	}

	const ULocalPlayer* LocalPlayer = GetGameInstance() ? GetGameInstance()->GetFirstGamePlayer() : nullptr;
	const FUniqueNetIdPtr UserId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId() : nullptr;
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DOSAchievementSubsystem: no valid user id, skipping unlock for '%s'."), *Key.ToString());
		return;
	}

	FOnlineAchievementsWriteRef WriteObject = MakeShared<FOnlineAchievementsWrite>();
	WriteObject->SetFloatStat(*Mapping->SteamAchievementId, 100.f);

	Achievements->WriteAchievements(*UserId, WriteObject);
}

void UDOSAchievementSubsystem::HandleEndingResolved(FName EndingId)
{
	// One-to-one convention: an ending's own FName doubles as its achievement key. Combo/count
	// based achievements (e.g. "reached every ending") should be layered on top separately
	// once that data exists, rather than baked in here.
	UnlockAchievement(EndingId);
}

void UDOSAchievementSubsystem::HandleClueDiscovered(FName ClueId)
{
	// Same convention as above - only fires for clues that map to their own achievement
	// (e.g. a rare/hidden clue). Most clues will simply have no mapping and no-op.
	UnlockAchievement(ClueId);
}
