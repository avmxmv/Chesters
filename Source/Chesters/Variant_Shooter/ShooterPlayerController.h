// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class AShooterCharacter;
class AShooterWeapon;
class UShooterBulletCounterUI;
class FLifetimeProperty;

USTRUCT(BlueprintType)
struct FShooterWeaponPurchaseOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Buy")
	TSubclassOf<AShooterWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Buy", meta=(ClampMin=0))
	int32 Price = 200;
};

/**
 *  Simple PlayerController for a first person shooter game
 *  Manages input mappings
 *  Respawns the player pawn when it's destroyed
 */
UCLASS(abstract, config="Game")
class CHESTERS_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input mapping contexts for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Character class to respawn when the possessed pawn is destroyed */
	UPROPERTY(EditAnywhere, Category="Shooter|Respawn")
	TSubclassOf<AShooterCharacter> CharacterClass;

	/** Money each player starts with in the current match */
	UPROPERTY(EditAnywhere, Category="Shooter|Buy", meta=(ClampMin=0))
	int32 StartingMoney = 800;

	/** Weapons available to buy during the match */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Buy", meta=(AllowPrivateAccess="true"))
	TArray<FShooterWeaponPurchaseOption> BuyMenuOptions;

	/** Current match money for this player */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentMoney, BlueprintReadOnly, Category="Shooter|Buy", meta=(AllowPrivateAccess="true"))
	int32 CurrentMoney = 0;

	/** Type of bullet counter UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter|UI")
	TSubclassOf<UShooterBulletCounterUI> BulletCounterUIClass;

	/** Tag to grant the possessed pawn to flag it as the player */
	UPROPERTY(EditAnywhere, Category="Shooter|Player")
	FName PlayerPawnTag = FName("Player");

	/** Pointer to the bullet counter UI widget */
	UPROPERTY()
	TObjectPtr<UShooterBulletCounterUI> BulletCounterUI;

protected:

	/** Replication setup */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Gameplay Initialization */
	virtual void BeginPlay() override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn) override;

	/** Called if the possessed pawn is destroyed */
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	/** Called when the bullet count on the possessed pawn is updated */
	UFUNCTION()
	void OnBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	/** Called when the possessed pawn is damaged */
	UFUNCTION()
	void OnPawnDamaged(float LifePercent);

	/** Called when the possessed pawn starts reloading */
	UFUNCTION()
	void OnReloadStarted(float ReloadDuration);

	/** Called when the possessed pawn finishes reloading */
	UFUNCTION()
	void OnReloadFinished();

	/** Called when replicated money changes on the owning client */
	UFUNCTION()
	void OnRep_CurrentMoney();

	/** Notifies local UI/Blueprints that money changed */
	void HandleMoneyChanged();

	/** Shows reload UI on the owning client */
	UFUNCTION(Client, Reliable)
	void ClientReloadStarted(float ReloadDuration);

	/** Hides reload UI on the owning client */
	UFUNCTION(Client, Reliable)
	void ClientReloadFinished();

	/** Local helper for starting reload UI */
	void HandleReloadStarted(float ReloadDuration);

	/** Local helper for finishing reload UI */
	void HandleReloadFinished();

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:

	/** Attempts to buy the weapon at the configured buy menu index */
	UFUNCTION(BlueprintCallable, Category="Shooter|Buy")
	void BuyWeapon(int32 OptionIndex);

	/** Returns the player's current match money */
	UFUNCTION(BlueprintPure, Category="Shooter|Buy")
	int32 GetCurrentMoney() const { return CurrentMoney; }

	/** Allows Blueprint UI to refresh money text */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter|Buy", meta=(DisplayName="On Money Changed"))
	void BP_OnMoneyChanged(int32 NewMoney);

protected:

	/** Server-authoritative buy request */
	UFUNCTION(Server, Reliable)
	void ServerBuyWeapon(int32 OptionIndex);
};
