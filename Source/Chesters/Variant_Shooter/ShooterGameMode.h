// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class AShooterCharacter;
class AShooterNPC;
class AShooterNPCSpawner;
class UShooterUI;

/**
 *  Simple GameMode for a first person shooter game
 *  Manages game UI
 *  Keeps track of team scores
 */
UCLASS(abstract)
class CHESTERS_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	/** Type of UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter")
	TSubclassOf<UShooterUI> ShooterUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UShooterUI> ShooterUI;

	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

	/** Teams that participate in shooter rounds. */
	UPROPERTY(EditAnywhere, Category="Shooter|Rounds")
	TArray<uint8> RoundTeams = { 0, 1 };

	/** Max round time before winner is decided by surviving team members. */
	UPROPERTY(EditAnywhere, Category="Shooter|Rounds", meta=(ClampMin=1, Units="s"))
	float RoundDuration = 60.0f;

	/** Delay between round end and next round start. */
	UPROPERTY(EditAnywhere, Category="Shooter|Rounds", meta=(ClampMin=0, Units="s"))
	float RoundRestartDelay = 5.0f;

	/** Current round number, starting from 1. */
	int32 CurrentRound = 0;

	/** True while the current round can still be won. */
	bool bRoundInProgress = false;

	/** True after the main menu has started gameplay. */
	bool bGameStarted = false;

	/** True after the first round has started. Used to avoid resetting initial actors twice. */
	bool bHasStartedAnyRound = false;

	/** Timer that ends the round by survivor count. */
	FTimerHandle RoundTimer;

	/** Timer that starts the next round after the end delay. */
	FTimerHandle RoundRestartTimer;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Called by pawns when a member of a team dies during a round. */
	void NotifyTeamMemberDied(uint8 TeamByte);

	/** Starts the round flow from the main menu. */
	UFUNCTION(BlueprintCallable, Category="Shooter|Rounds")
	void StartGameFromMenu();

	/** Returns true once gameplay has started from the main menu. */
	bool HasGameStarted() const { return bGameStarted; }

protected:

	/** Starts a new round. */
	void StartRound(bool bResetRoundActors);

	/** Ends the current round and optionally awards a winning team. */
	void EndRound(uint8 WinningTeam, bool bHasWinner);

	/** Ends the current round because the timer expired. */
	void EndRoundByTimer();

	/** Starts the next round after the restart delay. */
	void RestartRound();

	/** Returns the number of living shooter pawns on the given team. */
	int32 CountLivingTeamMembers(uint8 TeamByte) const;

	/** Finds a winner if all living members of one or more teams were eliminated. */
	bool FindEliminationWinner(uint8& OutWinningTeam) const;

	/** Finds a winner by comparing living team member counts. */
	bool FindSurvivorWinner(uint8& OutWinningTeam) const;

	/** Resets player pawns and NPC spawners for a new round. */
	void ResetRoundActors();
};
