// Single save slot for the whole game: an in-progress checkpoint restored by Continue, plus a
// permanent unlock flag that survives StartNewGame (see UDOSSaveGameSubsystem).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DOSSaveGame.generated.h"

UCLASS()
class DOS_API UDOSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Set once the true ending is reached; never cleared by StartNewGame.
	UPROPERTY()
	bool bTrueEndingUnlocked = false;

	// True while TimelineTimeSeconds/CurrentRound/ActiveInterventions/CollectedClues below
	// describe an in-progress case that Continue can resume. Cleared whenever an ending
	// resolves (the case is over, there is nothing left to continue into) and whenever a new
	// game starts fresh - see UDOSSaveGameSubsystem::HasSaveGame().
	UPROPERTY()
	bool bHasActiveCheckpoint = false;

	// Checkpoint captured whenever DOSCaseSubsystem::OnActiveInterventionsChanged fires.
	UPROPERTY()
	float TimelineTimeSeconds = 0.f;

	UPROPERTY()
	int32 CurrentRound = 1;

	UPROPERTY()
	TArray<FName> ActiveInterventions;

	UPROPERTY()
	TArray<FName> CollectedClues;
};
