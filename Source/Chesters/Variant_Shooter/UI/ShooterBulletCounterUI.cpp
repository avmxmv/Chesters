// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterBulletCounterUI.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"

void UShooterBulletCounterUI::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bReloading)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 UShooterBulletCounterUI::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 PaintedLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bReloading || ActiveReloadDuration <= 0.0f || !GetWorld())
	{
		return PaintedLayer;
	}

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center(Size.X * 0.5f, Size.Y * 0.5f);
	const float Elapsed = GetWorld()->GetTimeSeconds() - ReloadStartTime;
	const float Percent = FMath::Clamp(Elapsed / ActiveReloadDuration, 0.0f, 1.0f);

	const FLinearColor Red(1.0f, 0.0f, 0.0f, 1.0f);
	const FLinearColor DarkRed(0.25f, 0.0f, 0.0f, 0.85f);
	const float CrossHalfSize = 12.0f;
	const float CrossThickness = 3.0f;

	TArray<FVector2D> CrossLineA;
	CrossLineA.Add(Center + FVector2D(-CrossHalfSize, -CrossHalfSize));
	CrossLineA.Add(Center + FVector2D(CrossHalfSize, CrossHalfSize));
	FSlateDrawElement::MakeLines(OutDrawElements, PaintedLayer + 1, AllottedGeometry.ToPaintGeometry(), CrossLineA, ESlateDrawEffect::None, Red, true, CrossThickness);

	TArray<FVector2D> CrossLineB;
	CrossLineB.Add(Center + FVector2D(-CrossHalfSize, CrossHalfSize));
	CrossLineB.Add(Center + FVector2D(CrossHalfSize, -CrossHalfSize));
	FSlateDrawElement::MakeLines(OutDrawElements, PaintedLayer + 1, AllottedGeometry.ToPaintGeometry(), CrossLineB, ESlateDrawEffect::None, Red, true, CrossThickness);

	const float BarWidth = 180.0f;
	const float BarY = Center.Y + 38.0f;
	const FVector2D BarStart(Center.X - BarWidth * 0.5f, BarY);
	const FVector2D BarEnd(Center.X + BarWidth * 0.5f, BarY);
	const FVector2D FillEnd(BarStart.X + BarWidth * Percent, BarY);

	TArray<FVector2D> BarBackground;
	BarBackground.Add(BarStart);
	BarBackground.Add(BarEnd);
	FSlateDrawElement::MakeLines(OutDrawElements, PaintedLayer + 1, AllottedGeometry.ToPaintGeometry(), BarBackground, ESlateDrawEffect::None, DarkRed, true, 6.0f);

	TArray<FVector2D> BarFill;
	BarFill.Add(BarStart);
	BarFill.Add(FillEnd);
	FSlateDrawElement::MakeLines(OutDrawElements, PaintedLayer + 2, AllottedGeometry.ToPaintGeometry(), BarFill, ESlateDrawEffect::None, Red, true, 6.0f);

	return PaintedLayer + 2;
}

void UShooterBulletCounterUI::StartReloadIndicator(float InReloadDuration)
{
	ActiveReloadDuration = FMath::Max(InReloadDuration, KINDA_SMALL_NUMBER);
	ReloadStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bReloading = true;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UShooterBulletCounterUI::FinishReloadIndicator()
{
	bReloading = false;
	Invalidate(EInvalidateWidgetReason::Paint);
}
