// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/ShooterMainMenuPlayerController.h"
#include "Chesters.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Variant_Shooter/Weapons/ShooterProjectile.h"
#include "Variant_Shooter/Weapons/ShooterWeapon.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void AShooterMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowMainMenu();
}

void AShooterMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideMenuOverlays();

	Super::EndPlay(EndPlayReason);
}

void AShooterMainMenuPlayerController::ShowMainMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	HideMenuOverlays();

	if (GEngine && GEngine->GameViewport)
	{
		MainMenuOverlay = BuildMainMenuOverlay();
		GEngine->GameViewport->AddViewportWidgetContent(MainMenuOverlay.ToSharedRef(), 1000);
		SetMenuInputMode(MainMenuOverlay);
	}
	else
	{
		UE_LOG(LogChesters, Error, TEXT("Could not access the game viewport to show the main menu."));
	}
}

void AShooterMainMenuPlayerController::ShowWeaponInfoMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	HideMenuOverlays();

	if (GEngine && GEngine->GameViewport)
	{
		WeaponInfoOverlay = BuildWeaponInfoOverlay();
		GEngine->GameViewport->AddViewportWidgetContent(WeaponInfoOverlay.ToSharedRef(), 1000);
		SetMenuInputMode(WeaponInfoOverlay);
	}
	else
	{
		UE_LOG(LogChesters, Error, TEXT("Could not access the game viewport to show weapon info."));
	}
}

void AShooterMainMenuPlayerController::HideMenuOverlays()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		MainMenuOverlay.Reset();
		WeaponInfoOverlay.Reset();
		return;
	}

	if (MainMenuOverlay.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MainMenuOverlay.ToSharedRef());
		MainMenuOverlay.Reset();
	}

	if (WeaponInfoOverlay.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(WeaponInfoOverlay.ToSharedRef());
		WeaponInfoOverlay.Reset();
	}
}

void AShooterMainMenuPlayerController::SetMenuInputMode(TSharedPtr<SWidget> FocusWidget)
{
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (FocusWidget.IsValid())
	{
		InputMode.SetWidgetToFocus(FocusWidget);
	}

	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

TSharedRef<SWidget> AShooterMainMenuPlayerController::BuildMainMenuOverlay()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.015f, 0.02f, 1.0f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 40.0f)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Chesters")))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 56))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SBox)
				.WidthOverride(380.0f)
				.HeightOverride(68.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &AShooterMainMenuPlayerController::HandleStartGameClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Начать игру")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 30))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SBox)
				.WidthOverride(380.0f)
				.HeightOverride(68.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &AShooterMainMenuPlayerController::HandleWeaponInfoClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Виды оружия")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 30))
					]
				]
			]
		];
}

TSharedRef<SWidget> AShooterMainMenuPlayerController::BuildWeaponInfoOverlay()
{
	TArray<FShooterWeaponInfo> WeaponInfos;
	GetWeaponInfoList(WeaponInfos);

	TSharedRef<SVerticalBox> WeaponList = SNew(SVerticalBox);
	for (const FShooterWeaponInfo& WeaponInfo : WeaponInfos)
	{
		WeaponList->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 8.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.09f, 1.0f))
			.Padding(18.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(WeaponInfo.DisplayName)
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::Format(FText::FromString(TEXT("Урон: {0}    Скорострельность: {1} выстр./сек    Перезарядка: {2} сек")),
						FText::AsNumber(FMath::RoundToInt(WeaponInfo.Damage)),
						FText::AsNumber(WeaponInfo.FireRate),
						FText::AsNumber(WeaponInfo.ReloadDuration)))
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
				]
			]
		];
	}

	if (WeaponInfos.IsEmpty())
	{
		WeaponList->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 20.0f)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Список оружия пока не настроен")))
			.ColorAndOpacity(FSlateColor(FLinearColor::White))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 24))
		];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.015f, 0.02f, 1.0f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(980.0f)
			.HeightOverride(720.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 20.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Виды оружия")))
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						WeaponList
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 20.0f, 0.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(260.0f)
					.HeightOverride(58.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.OnClicked(FOnClicked::CreateUObject(this, &AShooterMainMenuPlayerController::HandleBackClicked))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Назад")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 26))
						]
					]
				]
			]
		];
}

void AShooterMainMenuPlayerController::GetWeaponInfoList(TArray<FShooterWeaponInfo>& OutWeaponInfos) const
{
	OutWeaponInfos.Reset();

	for (const FShooterWeaponDisplayOption& DisplayOption : WeaponInfoOptions)
	{
		if (!DisplayOption.WeaponClass)
		{
			continue;
		}

		const AShooterWeapon* WeaponDefaults = DisplayOption.WeaponClass->GetDefaultObject<AShooterWeapon>();
		if (!WeaponDefaults)
		{
			continue;
		}

		FShooterWeaponInfo WeaponInfo;
		WeaponInfo.DisplayName = DisplayOption.DisplayName.IsEmpty()
			? FText::FromString(DisplayOption.WeaponClass->GetName().Replace(TEXT("_C"), TEXT("")))
			: DisplayOption.DisplayName;
		WeaponInfo.WeaponClass = DisplayOption.WeaponClass;
		WeaponInfo.Icon = DisplayOption.Icon;
		WeaponInfo.ReloadDuration = WeaponDefaults->GetReloadDuration();
		WeaponInfo.FireRate = WeaponDefaults->GetRefireRate() > 0.0f ? 1.0f / WeaponDefaults->GetRefireRate() : 0.0f;

		if (TSubclassOf<AShooterProjectile> ProjectileClass = WeaponDefaults->GetProjectileClass())
		{
			if (const AShooterProjectile* ProjectileDefaults = ProjectileClass->GetDefaultObject<AShooterProjectile>())
			{
				WeaponInfo.Damage = ProjectileDefaults->GetHitDamage();
			}
		}

		OutWeaponInfos.Add(WeaponInfo);
	}
}

FReply AShooterMainMenuPlayerController::HandleStartGameClicked()
{
	HideMenuOverlays();
	UGameplayStatics::OpenLevel(this, ShooterMapName);
	return FReply::Handled();
}

FReply AShooterMainMenuPlayerController::HandleWeaponInfoClicked()
{
	ShowWeaponInfoMenu();
	return FReply::Handled();
}

FReply AShooterMainMenuPlayerController::HandleBackClicked()
{
	ShowMainMenu();
	return FReply::Handled();
}
