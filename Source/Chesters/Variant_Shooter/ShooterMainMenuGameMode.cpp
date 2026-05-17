// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/ShooterMainMenuGameMode.h"
#include "Variant_Shooter/ShooterMainMenuPlayerController.h"

AShooterMainMenuGameMode::AShooterMainMenuGameMode()
{
	PlayerControllerClass = AShooterMainMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}
