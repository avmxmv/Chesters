// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterUI.generated.h"

/**
 *  Simple scoreboard UI for a first person shooter game
 */
UCLASS(abstract)
class CHESTERS_API UShooterUI : public UUserWidget
{
	GENERATED_BODY()
	
public:

	/** Allows Blueprint to update score sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Update Score"))
	void BP_UpdateScore(uint8 TeamByte, int32 Score);

	/** Allows Blueprint to show round start state. */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Round Started"))
	void BP_RoundStarted(int32 RoundNumber, float RoundDuration);

	/** Allows Blueprint to show round end state. */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "Round Ended"))
	void BP_RoundEnded(uint8 WinningTeam, bool bHasWinner);
};
