// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterWeaponInfoTypes.h"
#include "ShooterWeaponInfoUI.generated.h"

class AShooterPlayerController;
class UButton;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS()
class CHESTERS_API UShooterWeaponInfoUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> WeaponListBox;

	UPROPERTY()
	TObjectPtr<UButton> BackButton;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleBackClicked();

	void RebuildWeaponList();
	UWidget* CreateWeaponCard(const FShooterWeaponInfo& WeaponInfo);
	UTextBlock* CreateTextBlock(const FText& Text, float FontSize) const;

	AShooterPlayerController* GetShooterPlayerController() const;
};
