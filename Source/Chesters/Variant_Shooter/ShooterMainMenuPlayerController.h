// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "UI/ShooterWeaponInfoTypes.h"
#include "ShooterMainMenuPlayerController.generated.h"

class SWidget;

UCLASS()
class CHESTERS_API AShooterMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category="Shooter|Main Menu")
	FName ShooterMapName = FName(TEXT("/Game/Variant_Shooter/Lvl_Shooter"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Main Menu", meta=(AllowPrivateAccess="true"))
	TArray<FShooterWeaponDisplayOption> WeaponInfoOptions;

	TSharedPtr<SWidget> MainMenuOverlay;
	TSharedPtr<SWidget> WeaponInfoOverlay;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ShowMainMenu();
	void ShowWeaponInfoMenu();
	void HideMenuOverlays();
	void SetMenuInputMode(TSharedPtr<SWidget> FocusWidget);

	TSharedRef<SWidget> BuildMainMenuOverlay();
	TSharedRef<SWidget> BuildWeaponInfoOverlay();
	void GetWeaponInfoList(TArray<FShooterWeaponInfo>& OutWeaponInfos) const;

	FReply HandleStartGameClicked();
	FReply HandleWeaponInfoClicked();
	FReply HandleBackClicked();
};
