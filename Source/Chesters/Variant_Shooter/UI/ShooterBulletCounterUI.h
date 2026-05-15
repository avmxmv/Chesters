// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterBulletCounterUI.generated.h"

/**
 *  Simple bullet counter UI widget for a first person shooter game
 */
UCLASS(abstract)
class CHESTERS_API UShooterBulletCounterUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	float ReloadStartTime = 0.0f;
	float ActiveReloadDuration = 0.0f;
	bool bReloading = false;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
public:

	/** Allows Blueprint to update sub-widgets with the new bullet count */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "UpdateBulletCounter"))
	void BP_UpdateBulletCounter(int32 MagazineSize, int32 BulletCount);

	/** Allows Blueprint to update sub-widgets with the new life total and play a damage effect on the HUD */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Damaged"))
	void BP_Damaged(float LifePercent);

	/** Allows Blueprint to show reload progress for the given duration */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Reload Started"))
	void BP_ReloadStarted(float ReloadDuration);

	/** Allows Blueprint to hide reload progress when reloading completes */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta=(DisplayName = "Reload Finished"))
	void BP_ReloadFinished();

	/** Starts native reload crosshair/progress rendering inside the existing HUD widget */
	void StartReloadIndicator(float InReloadDuration);

	/** Stops native reload crosshair/progress rendering inside the existing HUD widget */
	void FinishReloadIndicator();
};
