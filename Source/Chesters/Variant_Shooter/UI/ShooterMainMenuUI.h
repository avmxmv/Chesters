// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterMainMenuUI.generated.h"

class AShooterPlayerController;
class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class CHESTERS_API UShooterMainMenuUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TObjectPtr<UButton> StartGameButton;

	UPROPERTY()
	TObjectPtr<UButton> WeaponsButton;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleStartGameClicked();

	UFUNCTION()
	void HandleWeaponsClicked();

	UTextBlock* CreateTextBlock(const FText& Text, float FontSize) const;

	AShooterPlayerController* GetShooterPlayerController() const;
};
