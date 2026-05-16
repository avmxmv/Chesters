// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class AShooterProjectile;
class USkeletalMeshComponent;
class UPointLightComponent;
class UAnimMontage;
class UAnimInstance;
class FLifetimeProperty;

/**
 *  Base class for a simple first person shooter weapon
 *  Provides both first person and third person perspective meshes
 *  Handles ammo and firing logic
 *  Interacts with the weapon owner through the ShooterWeaponHolder interface
 */
UCLASS(abstract)
class CHESTERS_API AShooterWeapon : public AActor
{
	GENERATED_BODY()
	
	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

	/** Short-lived light used as a simple muzzle flash. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* MuzzleFlashLight;

protected:

	/** Cast pointer to the weapon owner */
	IShooterWeaponHolder* WeaponOwner;

	/** Type of projectiles this weapon will shoot */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Number of bullets in a magazine */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** Number of bullets in the current magazine */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentBullets)
	int32 CurrentBullets = 0;

	/** If true, this weapon is currently reloading */
	bool bIsReloading = false;
	
	/** Animation montage to play when firing this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** Animation montage to play when reloading this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* ReloadMontage;

	/** Time it takes to refill the magazine after pressing reload */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float ReloadDuration = 1.5f;

	/** AnimInstance class to set for the first person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Cone half-angle for variance while aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** Amount of firing recoil to apply to the owner */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** Name of the first person muzzle socket where projectiles will spawn */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	/** Distance ahead of the muzzle that bullets will spawn at */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** How long the muzzle flash light remains visible after a shot. */
	UPROPERTY(EditAnywhere, Category="Visuals|Muzzle Flash", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float MuzzleFlashDuration = 0.05f;

	/** Brightness of the muzzle flash light. */
	UPROPERTY(EditAnywhere, Category="Visuals|Muzzle Flash", meta = (ClampMin = 0, ClampMax = 10000))
	float MuzzleFlashIntensity = 6000.0f;

	/** Radius affected by the muzzle flash light. */
	UPROPERTY(EditAnywhere, Category="Visuals|Muzzle Flash", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleFlashRadius = 160.0f;

	/** Color of the muzzle flash light. */
	UPROPERTY(EditAnywhere, Category="Visuals|Muzzle Flash")
	FLinearColor MuzzleFlashColor = FLinearColor(1.0f, 0.35f, 0.02f);

	/** If true, this weapon will automatically fire at the refire rate */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** Time between shots for this weapon. Affects both full auto and semi auto modes */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** Game time of last shot fired, used to enforce refire rate on semi auto */
	float TimeOfLastShot = 0.0f;

	/** If true, the weapon is currently firing */
	bool bIsFiring = false;

	/** Timer to handle full auto refiring */
	FTimerHandle RefireTimer;

	/** Timer to hide the muzzle flash light. */
	FTimerHandle MuzzleFlashTimer;

	/** Timer to complete reload after ReloadDuration expires. */
	FTimerHandle ReloadTimer;

	/** Cast pawn pointer to the owner for AI perception system interactions */
	TObjectPtr<APawn> PawnOwner;

	/** Loudness of the shot for AI perception system interactions */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** Max range of shot AI perception noise */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 3000.0f;

	/** Tag to apply to noise generated by shooting this weapon */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName ShotNoiseTag = FName("Shot");

public:	

	/** Constructor */
	AShooterWeapon();

protected:
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Replication setup */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	/** Called when the weapon's owner is destroyed */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

	/** Called when ammo changes on clients */
	UFUNCTION()
	void OnRep_CurrentBullets();

public:

	/** Activates this weapon and gets it ready to fire */
	void ActivateWeapon();

	/** Deactivates this weapon */
	void DeactivateWeapon();

	/** Start firing this weapon */
	void StartFiring();

	/** Stop firing this weapon */
	void StopFiring();

	/** Reloads this weapon's magazine */
	void Reload();

protected:

	/** Fire the weapon */
	virtual void Fire();

	/** Called when the refire rate time has passed while shooting semi auto weapons */
	void FireCooldownExpired();

	/** Completes the pending reload */
	void FinishReload();

	/** Fire a projectile towards the target location */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** Calculates the spawn transform for projectiles shot by this weapon */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

	/** Shows the muzzle flash light briefly at the muzzle socket. */
	void TriggerMuzzleFlash();

	/** Hides the muzzle flash light after its timer expires. */
	void HideMuzzleFlash();

public:

	/** Returns the first person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Returns the first person anim instance class */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** Returns the magazine size */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** Returns the current bullet count */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** Returns true if this weapon can start reloading now */
	bool CanReload() const { return !bIsReloading && CurrentBullets < MagazineSize; }

	/** Returns how long reload takes for this weapon */
	float GetReloadDuration() const { return ReloadDuration; }
};
