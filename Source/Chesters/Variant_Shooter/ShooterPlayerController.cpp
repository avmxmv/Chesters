// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "ShooterGameMode.h"
#include "ShooterMainMenuUI.h"
#include "ShooterProjectile.h"
#include "ShooterWeapon.h"
#include "ShooterWeaponInfoUI.h"
#include "Chesters.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Net/UnrealNetwork.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CurrentMoney = StartingMoney;
		HandleMoneyChanged();
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogChesters, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}

		// create the bullet counter widget and add it to the screen
		BulletCounterUI = CreateWidget<UShooterBulletCounterUI>(this, BulletCounterUIClass);

		if (BulletCounterUI)
		{
			BulletCounterUI->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogChesters, Error, TEXT("Could not spawn bullet counter widget."));

		}

		HandleMoneyChanged();
		SetGameInputMode();
	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

	// is this a shooter character?
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{
		// add the player tag
		ShooterCharacter->Tags.Add(PlayerPawnTag);

		// subscribe to the pawn's delegates
		ShooterCharacter->OnBulletCountUpdated.AddDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
		ShooterCharacter->OnDamaged.AddDynamic(this, &AShooterPlayerController::OnPawnDamaged);
		ShooterCharacter->OnReloadStarted.AddDynamic(this, &AShooterPlayerController::OnReloadStarted);
		ShooterCharacter->OnReloadFinished.AddDynamic(this, &AShooterPlayerController::OnReloadFinished);

		// force update the life bar
		ShooterCharacter->OnDamaged.Broadcast(1.0f);
	}

	SetGameInputMode();
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// reset the bullet counter HUD
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_UpdateBulletCounter(0, 0);
		BulletCounterUI->BP_ReloadFinished();
	}

	if (!HasAuthority())
	{
		return;
	}

	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// select a random player start
		AActor* RandomPlayerStart = ActorList[FMath::RandRange(0, ActorList.Num() - 1)];

		// spawn a character at the player start
		const FTransform SpawnTransform = RandomPlayerStart->GetActorTransform();

		if (AShooterCharacter* RespawnedCharacter = GetWorld()->SpawnActor<AShooterCharacter>(CharacterClass, SpawnTransform))
		{
			// possess the character
			Possess(RespawnedCharacter);
		}
	}
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	// update the UI
	if (BulletCounterUI)
	{
		BulletCounterUI->BP_UpdateBulletCounter(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->BP_Damaged(LifePercent);
	}
}

void AShooterPlayerController::OnReloadStarted(float ReloadDuration)
{
	HandleReloadStarted(ReloadDuration);

	if (HasAuthority() && !IsLocalPlayerController())
	{
		ClientReloadStarted(ReloadDuration);
	}
}

void AShooterPlayerController::OnReloadFinished()
{
	HandleReloadFinished();

	if (HasAuthority() && !IsLocalPlayerController())
	{
		ClientReloadFinished();
	}
}

void AShooterPlayerController::ClientReloadStarted_Implementation(float ReloadDuration)
{
	HandleReloadStarted(ReloadDuration);
}

void AShooterPlayerController::ClientReloadFinished_Implementation()
{
	HandleReloadFinished();
}

void AShooterPlayerController::HandleReloadStarted(float ReloadDuration)
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->StartReloadIndicator(ReloadDuration);
		BulletCounterUI->BP_ReloadStarted(ReloadDuration);
	}
}

void AShooterPlayerController::HandleReloadFinished()
{
	if (IsValid(BulletCounterUI))
	{
		BulletCounterUI->FinishReloadIndicator();
		BulletCounterUI->BP_ReloadFinished();
	}
}

void AShooterPlayerController::OnRep_CurrentMoney()
{
	HandleMoneyChanged();
}

void AShooterPlayerController::HandleMoneyChanged()
{
	if (IsLocalPlayerController())
	{
		BP_OnMoneyChanged(CurrentMoney);
	}
}

bool AShooterPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AShooterPlayerController::SetMenuInputMode(TSharedPtr<SWidget> FocusWidget)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (FocusWidget.IsValid())
	{
		InputMode.SetWidgetToFocus(FocusWidget);
	}

	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void AShooterPlayerController::SetGameInputMode()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void AShooterPlayerController::HideMenuOverlays()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	if (WeaponInfoWidget)
	{
		WeaponInfoWidget->RemoveFromParent();
		WeaponInfoWidget = nullptr;
	}

	if (GEngine && GEngine->GameViewport)
	{
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
}

TSharedRef<SWidget> AShooterPlayerController::BuildMainMenuOverlay()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.94f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 36.0f)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Chesters")))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 52))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SBox)
				.WidthOverride(360.0f)
				.HeightOverride(64.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &AShooterPlayerController::HandleSlateStartGameClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Начать игру")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SBox)
				.WidthOverride(360.0f)
				.HeightOverride(64.0f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked(FOnClicked::CreateUObject(this, &AShooterPlayerController::HandleSlateWeaponsClicked))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Виды оружия")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28))
					]
				]
			]
		];
}

TSharedRef<SWidget> AShooterPlayerController::BuildWeaponInfoOverlay()
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
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.09f, 0.95f))
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
			.Text(FText::FromString(TEXT("Список оружия пока пуст")))
			.ColorAndOpacity(FSlateColor(FLinearColor::White))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 24))
		];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.96f))
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
						.OnClicked(FOnClicked::CreateUObject(this, &AShooterPlayerController::HandleSlateBackClicked))
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

FReply AShooterPlayerController::HandleSlateStartGameClicked()
{
	StartGameFromMainMenu();
	return FReply::Handled();
}

FReply AShooterPlayerController::HandleSlateWeaponsClicked()
{
	ShowWeaponInfoMenu();
	return FReply::Handled();
}

FReply AShooterPlayerController::HandleSlateBackClicked()
{
	ShowMainMenu();
	return FReply::Handled();
}

void AShooterPlayerController::ShowMainMenu()
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
		SetGameInputMode();
		UE_LOG(LogChesters, Error, TEXT("Could not access the game viewport to show the main menu."));
	}
}

void AShooterPlayerController::ShowWeaponInfoMenu()
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
		SetGameInputMode();
		UE_LOG(LogChesters, Error, TEXT("Could not access the game viewport to show weapon info."));
	}
}

void AShooterPlayerController::StartGameFromMainMenu()
{
	HideMenuOverlays();
	SetGameInputMode();

	if (HasAuthority())
	{
		ServerStartGameFromMainMenu_Implementation();
	}
	else
	{
		ServerStartGameFromMainMenu();
	}
}

void AShooterPlayerController::GetWeaponInfoList(TArray<FShooterWeaponInfo>& OutWeaponInfos) const
{
	OutWeaponInfos.Reset();

	TArray<FShooterWeaponDisplayOption> DisplayOptions = WeaponInfoOptions;
	if (DisplayOptions.IsEmpty())
	{
		for (const FShooterWeaponPurchaseOption& BuyOption : BuyMenuOptions)
		{
			FShooterWeaponDisplayOption DisplayOption;
			DisplayOption.WeaponClass = BuyOption.WeaponClass;
			DisplayOptions.Add(DisplayOption);
		}
	}

	for (const FShooterWeaponDisplayOption& DisplayOption : DisplayOptions)
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
				WeaponInfo.bExplosive = ProjectileDefaults->ExplodesOnHit();
				WeaponInfo.ExplosionRadius = ProjectileDefaults->GetExplosionRadius();
			}
		}

		OutWeaponInfos.Add(WeaponInfo);
	}
}

void AShooterPlayerController::BuyWeapon(int32 OptionIndex)
{
	if (HasAuthority())
	{
		ServerBuyWeapon_Implementation(OptionIndex);
	}
	else
	{
		ServerBuyWeapon(OptionIndex);
	}
}

void AShooterPlayerController::ServerBuyWeapon_Implementation(int32 OptionIndex)
{
	if (!BuyMenuOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FShooterWeaponPurchaseOption& PurchaseOption = BuyMenuOptions[OptionIndex];
	if (!PurchaseOption.WeaponClass || CurrentMoney < PurchaseOption.Price)
	{
		return;
	}

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(GetPawn());
	if (!ShooterCharacter || ShooterCharacter->OwnsWeaponOfType(PurchaseOption.WeaponClass))
	{
		return;
	}

	CurrentMoney -= PurchaseOption.Price;
	HandleMoneyChanged();

	ShooterCharacter->AddWeaponClass(PurchaseOption.WeaponClass);
}

void AShooterPlayerController::ServerStartGameFromMainMenu_Implementation()
{
	if (AShooterGameMode* ShooterGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : nullptr)
	{
		ShooterGameMode->StartGameFromMenu();
	}
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, CurrentMoney);
}
