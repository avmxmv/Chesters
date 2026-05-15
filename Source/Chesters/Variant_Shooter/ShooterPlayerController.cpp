// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "ShooterWeapon.h"
#include "Chesters.h"
#include "Widgets/Input/SVirtualJoystick.h"
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

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, CurrentMoney);
}
