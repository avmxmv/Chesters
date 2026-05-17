// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterWeaponInfoUI.h"
#include "Variant_Shooter/ShooterPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

void UShooterWeaponInfoUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* CanvasRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WeaponInfoCanvas"));
	WidgetTree->RootWidget = CanvasRoot;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WeaponInfoBackground"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.94f));
	if (UCanvasPanelSlot* BackgroundSlot = CanvasRoot->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponInfoRoot"));
	if (UCanvasPanelSlot* RootBoxSlot = CanvasRoot->AddChildToCanvas(RootBox))
	{
		RootBoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		RootBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RootBoxSlot->SetSize(FVector2D(900.0f, 700.0f));
	}

	if (UTextBlock* TitleText = CreateTextBlock(FText::FromString(TEXT("Виды оружия")), 42.0f))
	{
		if (UVerticalBoxSlot* TitleBoxSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
			TitleBoxSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("WeaponInfoScroll"));
	WeaponListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeaponList"));
	ScrollBox->AddChild(WeaponListBox);
	if (UVerticalBoxSlot* ScrollBoxSlot = RootBox->AddChildToVerticalBox(ScrollBox))
	{
		ScrollBoxSlot->SetPadding(FMargin(80.0f, 0.0f));
	}

	BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
	BackButton->OnClicked.AddDynamic(this, &UShooterWeaponInfoUI::HandleBackClicked);
	BackButton->AddChild(CreateTextBlock(FText::FromString(TEXT("Назад")), 26.0f));
	if (UVerticalBoxSlot* BackButtonSlot = RootBox->AddChildToVerticalBox(BackButton))
	{
		BackButtonSlot->SetPadding(FMargin(120.0f, 20.0f));
	}

	RebuildWeaponList();
}

void UShooterWeaponInfoUI::HandleBackClicked()
{
	if (AShooterPlayerController* ShooterPlayerController = GetShooterPlayerController())
	{
		ShooterPlayerController->ShowMainMenu();
	}
}

void UShooterWeaponInfoUI::RebuildWeaponList()
{
	if (!WeaponListBox)
	{
		return;
	}

	WeaponListBox->ClearChildren();

	if (AShooterPlayerController* ShooterPlayerController = GetShooterPlayerController())
	{
		TArray<FShooterWeaponInfo> WeaponInfos;
		ShooterPlayerController->GetWeaponInfoList(WeaponInfos);

		for (const FShooterWeaponInfo& WeaponInfo : WeaponInfos)
		{
			if (UWidget* WeaponCard = CreateWeaponCard(WeaponInfo))
			{
				if (UVerticalBoxSlot* WeaponCardSlot = WeaponListBox->AddChildToVerticalBox(WeaponCard))
				{
					WeaponCardSlot->SetPadding(FMargin(0.0f, 8.0f));
				}
			}
		}
	}
}

UWidget* UShooterWeaponInfoUI::CreateWeaponCard(const FShooterWeaponInfo& WeaponInfo)
{
	UHorizontalBox* CardBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	if (WeaponInfo.Icon)
	{
		UImage* WeaponImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush Brush;
		Brush.SetResourceObject(WeaponInfo.Icon);
		Brush.ImageSize = FVector2D(128.0f, 128.0f);
		WeaponImage->SetBrush(Brush);

		if (UHorizontalBoxSlot* WeaponImageSlot = CardBox->AddChildToHorizontalBox(WeaponImage))
		{
			WeaponImageSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
			WeaponImageSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	else if (UTextBlock* PlaceholderText = CreateTextBlock(FText::FromString(TEXT("[иконка]")), 22.0f))
	{
		if (UHorizontalBoxSlot* PlaceholderSlot = CardBox->AddChildToHorizontalBox(PlaceholderText))
		{
			PlaceholderSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
			PlaceholderSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	UVerticalBox* StatsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	StatsBox->AddChildToVerticalBox(CreateTextBlock(WeaponInfo.DisplayName, 30.0f));
	StatsBox->AddChildToVerticalBox(CreateTextBlock(FText::Format(FText::FromString(TEXT("Урон: {0}")), FText::AsNumber(FMath::RoundToInt(WeaponInfo.Damage))), 22.0f));
	StatsBox->AddChildToVerticalBox(CreateTextBlock(FText::Format(FText::FromString(TEXT("Скорострельность: {0} выстр./сек")), FText::AsNumber(WeaponInfo.FireRate)), 22.0f));
	StatsBox->AddChildToVerticalBox(CreateTextBlock(FText::Format(FText::FromString(TEXT("Перезарядка: {0} сек")), FText::AsNumber(WeaponInfo.ReloadDuration)), 22.0f));
	if (WeaponInfo.bExplosive)
	{
		StatsBox->AddChildToVerticalBox(CreateTextBlock(FText::Format(FText::FromString(TEXT("Радиус взрыва: {0}")), FText::AsNumber(FMath::RoundToInt(WeaponInfo.ExplosionRadius))), 22.0f));
	}

	CardBox->AddChildToHorizontalBox(StatsBox);
	return CardBox;
}

UTextBlock* UShooterWeaponInfoUI::CreateTextBlock(const FText& Text, float FontSize) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);

	return TextBlock;
}

AShooterPlayerController* UShooterWeaponInfoUI::GetShooterPlayerController() const
{
	return Cast<AShooterPlayerController>(GetOwningPlayer());
}
