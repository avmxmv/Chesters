// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterMainMenuUI.h"
#include "Variant_Shooter/ShooterPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"

void UShooterMainMenuUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* CanvasRoot = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainMenuCanvas"));
	WidgetTree->RootWidget = CanvasRoot;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainMenuBackground"));
	Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.92f));
	if (UCanvasPanelSlot* BackgroundSlot = CanvasRoot->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuRoot"));
	if (UCanvasPanelSlot* RootBoxSlot = CanvasRoot->AddChildToCanvas(RootBox))
	{
		RootBoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		RootBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		RootBoxSlot->SetAutoSize(true);
	}

	if (UTextBlock* TitleText = CreateTextBlock(FText::FromString(TEXT("Chesters")), 48.0f))
	{
		if (UVerticalBoxSlot* TitleBoxSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleBoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 32.0f));
			TitleBoxSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	StartGameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartGameButton"));
	StartGameButton->OnClicked.AddDynamic(this, &UShooterMainMenuUI::HandleStartGameClicked);
	StartGameButton->AddChild(CreateTextBlock(FText::FromString(TEXT("Начать игру")), 28.0f));
	if (UVerticalBoxSlot* StartButtonSlot = RootBox->AddChildToVerticalBox(StartGameButton))
	{
		StartButtonSlot->SetPadding(FMargin(120.0f, 8.0f));
	}

	WeaponsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WeaponsButton"));
	WeaponsButton->OnClicked.AddDynamic(this, &UShooterMainMenuUI::HandleWeaponsClicked);
	WeaponsButton->AddChild(CreateTextBlock(FText::FromString(TEXT("Виды оружия")), 28.0f));
	if (UVerticalBoxSlot* WeaponsButtonSlot = RootBox->AddChildToVerticalBox(WeaponsButton))
	{
		WeaponsButtonSlot->SetPadding(FMargin(120.0f, 8.0f));
	}
}

void UShooterMainMenuUI::HandleStartGameClicked()
{
	if (AShooterPlayerController* ShooterPlayerController = GetShooterPlayerController())
	{
		ShooterPlayerController->StartGameFromMainMenu();
	}
}

void UShooterMainMenuUI::HandleWeaponsClicked()
{
	if (AShooterPlayerController* ShooterPlayerController = GetShooterPlayerController())
	{
		ShooterPlayerController->ShowWeaponInfoMenu();
	}
}

UTextBlock* UShooterMainMenuUI::CreateTextBlock(const FText& Text, float FontSize) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);
	TextBlock->SetJustification(ETextJustify::Center);

	return TextBlock;
}

AShooterPlayerController* UShooterMainMenuUI::GetShooterPlayerController() const
{
	return Cast<AShooterPlayerController>(GetOwningPlayer());
}
