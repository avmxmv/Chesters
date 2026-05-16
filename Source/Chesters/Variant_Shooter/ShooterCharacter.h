// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChestersCharacter.h"
#include "ShooterWeaponHolder.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReloadStartedDelegate, float, ReloadDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReloadFinishedDelegate);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class CHESTERS_API AShooterCharacter : public AChestersCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Movement speed while Shift is held */
	UPROPERTY(EditAnywhere, Category ="Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float SlowWalkSpeed = 250.0f;

	/** Cached movement speed restored when Shift is released */
	float DefaultWalkSpeed = 0.0f;

	/** Adds subtle first-person hand movement while running without Shift. */
	UPROPERTY(EditAnywhere, Category="Movement|First Person Run Bob")
	bool bEnableRunHandBob = true;

	/** Minimum speed required to play first-person hand bob. Keep above slow-walk speed. */
	UPROPERTY(EditAnywhere, Category="Movement|First Person Run Bob", meta = (ClampMin = 0, Units = "cm/s"))
	float RunHandBobSpeedThreshold = 300.0f;

	/** Frequency of first-person hand bob. */
	UPROPERTY(EditAnywhere, Category="Movement|First Person Run Bob", meta = (ClampMin = 0, ClampMax = 30))
	float RunHandBobFrequency = 9.0f;

	/** Local movement amplitude for first-person hand bob. */
	UPROPERTY(EditAnywhere, Category="Movement|First Person Run Bob")
	FVector RunHandBobLocationAmplitude = FVector(0.0f, 1.5f, 1.0f);

	/** Local rotation amplitude for first-person hand bob. */
	UPROPERTY(EditAnywhere, Category="Movement|First Person Run Bob")
	FRotator RunHandBobRotationAmplitude = FRotator(1.0f, 0.0f, 1.5f);

	FVector FirstPersonMeshBaseLocation = FVector::ZeroVector;
	FRotator FirstPersonMeshBaseRotation = FRotator::ZeroRotator;
	float RunHandBobTime = 0.0f;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** List of weapons picked up by the character */
	UPROPERTY(ReplicatedUsing=OnRep_OwnedWeapons)
	TArray<TObjectPtr<AShooterWeapon>> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon)
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	/** Optional weapon automatically granted when this character spawns */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapons", meta=(AllowPrivateAccess="true"))
	TSubclassOf<AShooterWeapon> StartingWeaponClass;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

	/** Reload started delegate */
	FReloadStartedDelegate OnReloadStarted;

	/** Reload finished delegate */
	FReloadFinishedDelegate OnReloadFinished;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Per-frame first-person visual updates */
	virtual void Tick(float DeltaSeconds) override;

	/** Replication setup */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Handles direct weapon slot input. Slot indexes are zero-based. */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSelectWeaponSlot(int32 WeaponIndex);

	/** Handles weapon slot 1 input */
	void DoSelectWeaponSlot1();

	/** Handles weapon slot 2 input */
	void DoSelectWeaponSlot2();

	/** Handles weapon slot 3 input */
	void DoSelectWeaponSlot3();

	/** Handles weapon slot 4 input */
	void DoSelectWeaponSlot4();

	/** Handles start slow-walk input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartSlowWalk();

	/** Handles stop slow-walk input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopSlowWalk();

	/** Handles reload input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReload();

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	/** Notifies the owner that this weapon started reloading */
	virtual void OnWeaponReloadStarted(float ReloadDuration) override;

	/** Notifies the owner that this weapon finished reloading */
	virtual void OnWeaponReloadFinished() override;

	//~End IShooterWeaponHolder interface

protected:

	/** Called when the current weapon changes on clients */
	UFUNCTION()
	void OnRep_CurrentWeapon();

	/** Called when the owned weapon list changes on clients */
	UFUNCTION()
	void OnRep_OwnedWeapons();

	/** Attaches owned weapons and shows only the currently equipped one */
	void RefreshWeaponAttachments();

	/** Server-authoritative start firing request */
	UFUNCTION(Server, Reliable)
	void ServerStartFiring();

	/** Server-authoritative stop firing request */
	UFUNCTION(Server, Reliable)
	void ServerStopFiring();

	/** Server-authoritative weapon switch request */
	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapon();

	/** Server-authoritative direct weapon slot request */
	UFUNCTION(Server, Reliable)
	void ServerSelectWeaponSlot(int32 WeaponIndex);

	/** Server-authoritative start slow-walk request */
	UFUNCTION(Server, Reliable)
	void ServerStartSlowWalk();

	/** Server-authoritative stop slow-walk request */
	UFUNCTION(Server, Reliable)
	void ServerStopSlowWalk();

	/** Server-authoritative reload request */
	UFUNCTION(Server, Reliable)
	void ServerReload();

	/** Applies or clears the slow-walk movement speed */
	void ApplySlowWalk(bool bSlowWalk);

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

public:

	/** Returns true if the character is dead */
	bool IsDead() const;

	/** Returns true if this character owns a weapon of the given class */
	UFUNCTION(BlueprintPure, Category="Weapons")
	bool OwnsWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;
};
