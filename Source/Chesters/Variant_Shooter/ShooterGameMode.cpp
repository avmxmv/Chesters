// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "Variant_Shooter/AI/ShooterNPC.h"
#include "Variant_Shooter/AI/ShooterNPCSpawner.h"
#include "Variant_Shooter/ShooterCharacter.h"
#include "ShooterUI.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// create the UI
	if (ShooterUIClass)
	{
		ShooterUI = CreateWidget<UShooterUI>(UGameplayStatics::GetPlayerController(GetWorld(), 0), ShooterUIClass);
		if (ShooterUI)
		{
			ShooterUI->AddToViewport(0);
		}
	}

	StartGameFromMenu();
}

void AShooterGameMode::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(RoundTimer);
	GetWorld()->GetTimerManager().ClearTimer(RoundRestartTimer);
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);

	// update the UI
	if (ShooterUI)
	{
		ShooterUI->BP_UpdateScore(TeamByte, Score);
	}
}

void AShooterGameMode::NotifyTeamMemberDied(uint8 TeamByte)
{
	if (!bRoundInProgress)
	{
		return;
	}

	uint8 WinningTeam = 0;
	if (FindEliminationWinner(WinningTeam))
	{
		EndRound(WinningTeam, true);
	}
}

void AShooterGameMode::StartGameFromMenu()
{
	if (bGameStarted)
	{
		return;
	}

	bGameStarted = true;
	StartRound(false);
}

void AShooterGameMode::StartRound(bool bResetRoundActors)
{
	++CurrentRound;
	bRoundInProgress = true;

	GetWorld()->GetTimerManager().ClearTimer(RoundRestartTimer);
	GetWorld()->GetTimerManager().SetTimer(RoundTimer, this, &AShooterGameMode::EndRoundByTimer, RoundDuration, false);

	if (bResetRoundActors || bHasStartedAnyRound)
	{
		ResetRoundActors();
	}
	else
	{
		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPCSpawner::StaticClass(), Spawners);
		for (AActor* Actor : Spawners)
		{
			if (AShooterNPCSpawner* Spawner = Cast<AShooterNPCSpawner>(Actor))
			{
				Spawner->ResetForRound(0.0f);
			}
		}
	}

	bHasStartedAnyRound = true;

	if (ShooterUI)
	{
		ShooterUI->BP_RoundStarted(CurrentRound, RoundDuration);
	}
}

void AShooterGameMode::EndRound(uint8 WinningTeam, bool bHasWinner)
{
	if (!bRoundInProgress)
	{
		return;
	}

	bRoundInProgress = false;
	GetWorld()->GetTimerManager().ClearTimer(RoundTimer);

	if (bHasWinner)
	{
		IncrementTeamScore(WinningTeam);
	}

	TArray<AActor*> PlayerCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), PlayerCharacters);
	for (AActor* Actor : PlayerCharacters)
	{
		if (AShooterCharacter* Character = Cast<AShooterCharacter>(Actor))
		{
			Character->StopCombatActions();
			Character->DisableInput(nullptr);
		}
	}

	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPCSpawner::StaticClass(), Spawners);
	for (AActor* Actor : Spawners)
	{
		if (AShooterNPCSpawner* Spawner = Cast<AShooterNPCSpawner>(Actor))
		{
			Spawner->StopSpawning();
		}
	}

	if (ShooterUI)
	{
		ShooterUI->BP_RoundEnded(WinningTeam, bHasWinner);
	}

	GetWorld()->GetTimerManager().SetTimer(RoundRestartTimer, this, &AShooterGameMode::RestartRound, RoundRestartDelay, false);
}

void AShooterGameMode::EndRoundByTimer()
{
	uint8 WinningTeam = 0;
	const bool bHasWinner = FindSurvivorWinner(WinningTeam);
	EndRound(WinningTeam, bHasWinner);
}

void AShooterGameMode::RestartRound()
{
	StartRound(true);
}

int32 AShooterGameMode::CountLivingTeamMembers(uint8 TeamByte) const
{
	int32 LivingCount = 0;

	TArray<AActor*> PlayerCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), PlayerCharacters);
	for (AActor* Actor : PlayerCharacters)
	{
		const AShooterCharacter* Character = Cast<AShooterCharacter>(Actor);
		if (Character && Character->GetTeamByte() == TeamByte && !Character->IsDead())
		{
			++LivingCount;
		}
	}

	TArray<AActor*> NPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPC::StaticClass(), NPCs);
	for (AActor* Actor : NPCs)
	{
		const AShooterNPC* NPC = Cast<AShooterNPC>(Actor);
		if (NPC && NPC->GetTeamByte() == TeamByte && !NPC->IsDead())
		{
			++LivingCount;
		}
	}

	return LivingCount;
}

bool AShooterGameMode::FindEliminationWinner(uint8& OutWinningTeam) const
{
	int32 TeamsAlive = 0;
	uint8 LastLivingTeam = 0;

	for (uint8 Team : RoundTeams)
	{
		if (CountLivingTeamMembers(Team) > 0)
		{
			++TeamsAlive;
			LastLivingTeam = Team;
		}
	}

	if (TeamsAlive == 1)
	{
		OutWinningTeam = LastLivingTeam;
		return true;
	}

	return false;
}

bool AShooterGameMode::FindSurvivorWinner(uint8& OutWinningTeam) const
{
	int32 BestLivingCount = INDEX_NONE;
	bool bTie = false;

	for (uint8 Team : RoundTeams)
	{
		const int32 LivingCount = CountLivingTeamMembers(Team);
		if (LivingCount > BestLivingCount)
		{
			BestLivingCount = LivingCount;
			OutWinningTeam = Team;
			bTie = false;
		}
		else if (LivingCount == BestLivingCount)
		{
			bTie = true;
		}
	}

	return !bTie && BestLivingCount > INDEX_NONE;
}

void AShooterGameMode::ResetRoundActors()
{
	TArray<AActor*> PlayerCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), PlayerCharacters);
	for (AActor* Actor : PlayerCharacters)
	{
		if (AShooterCharacter* Character = Cast<AShooterCharacter>(Actor))
		{
			Character->PrepareForRoundReset();
			Character->Destroy();
		}
	}

	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPCSpawner::StaticClass(), Spawners);
	for (AActor* Actor : Spawners)
	{
		if (AShooterNPCSpawner* Spawner = Cast<AShooterNPCSpawner>(Actor))
		{
			Spawner->ResetForRound(0.0f);
		}
	}
}
