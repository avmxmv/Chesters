// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

AShooterWeapon::AShooterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetActorHiddenInGame(true);

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the first person mesh
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);

	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->bOnlyOwnerSee = true;
	FirstPersonMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// create the third person mesh
	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Mesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);

	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->bOwnerNoSee = true;

	MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Muzzle Flash Light"));
	MuzzleFlashLight->SetupAttachment(FirstPersonMesh);
	MuzzleFlashLight->SetVisibility(false);
	MuzzleFlashLight->SetHiddenInGame(true);
	MuzzleFlashLight->SetCastShadows(false);
	MuzzleFlashLight->SetIntensity(MuzzleFlashIntensity);
	MuzzleFlashLight->SetAttenuationRadius(MuzzleFlashRadius);
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner())
	{
		return;
	}

	// subscribe to the owner's destroyed delegate
	GetOwner()->OnDestroyed.AddDynamic(this, &AShooterWeapon::OnOwnerDestroyed);

	// cast the weapon owner
	WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	if (HasAuthority())
	{
		// fill the first ammo clip
		CurrentBullets = MagazineSize;
	}

	// attach the meshes to the owner
	if (WeaponOwner)
	{
		WeaponOwner->AttachWeaponMeshes(this);
	}
}

void AShooterWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
	GetWorld()->GetTimerManager().ClearTimer(MuzzleFlashTimer);
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
}

void AShooterWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterWeapon, CurrentBullets);
}

void AShooterWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	// ensure this weapon is destroyed when the owner is destroyed
	Destroy();
}

void AShooterWeapon::OnRep_CurrentBullets()
{
	if (!WeaponOwner)
	{
		WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	}

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	}
}

void AShooterWeapon::ActivateWeapon()
{
	// unhide this weapon
	SetActorHiddenInGame(false);

	// notify the owner
	if (WeaponOwner)
	{
		WeaponOwner->OnWeaponActivated(this);
	}
}

void AShooterWeapon::DeactivateWeapon()
{
	// ensure we're no longer firing this weapon while deactivated
	StopFiring();

	// hide the weapon
	SetActorHiddenInGame(true);

	// notify the owner
	if (WeaponOwner)
	{
		WeaponOwner->OnWeaponDeactivated(this);
	}
}

void AShooterWeapon::StartFiring()
{
	if (!HasAuthority())
	{
		return;
	}

	// raise the firing flag
	bIsFiring = true;

	// check how much time has passed since we last shot
	// this may be under the refire rate if the weapon shoots slow enough and the player is spamming the trigger
	const float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - TimeOfLastShot;

	if (TimeSinceLastShot > RefireRate)
	{
		// fire the weapon right away
		Fire();

	} else {

		// if we're full auto, schedule the next shot
		if (bFullAuto)
		{
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, TimeSinceLastShot, false);
		}

	}
}

void AShooterWeapon::StopFiring()
{
	// lower the firing flag
	bIsFiring = false;

	// clear the refire timer
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::Reload()
{
	if (!HasAuthority() || bIsReloading || CurrentBullets >= MagazineSize)
	{
		return;
	}

	StopFiring();
	bIsReloading = true;

	if (WeaponOwner)
	{
		UAnimMontage* MontageToPlay = ReloadMontage;
		if (!MontageToPlay)
		{
			MontageToPlay = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload.MM_Pistol_Reload"));
		}

		if (MontageToPlay)
		{
			WeaponOwner->PlayFiringMontage(MontageToPlay);
		}

		WeaponOwner->OnWeaponReloadStarted(ReloadDuration);
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AShooterWeapon::FinishReload, ReloadDuration, false);
}

void AShooterWeapon::Fire()
{
	if (!HasAuthority())
	{
		return;
	}

	// ensure the player still wants to fire. They may have let go of the trigger
	if (!bIsFiring || bIsReloading || CurrentBullets <= 0)
	{
		return;
	}
	
	// fire a projectile at the target
	FireProjectile(WeaponOwner->GetWeaponTargetLocation());

	// update the time of our last shot
	TimeOfLastShot = GetWorld()->GetTimeSeconds();

	// make noise so the AI perception system can hear us
	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, ShotNoiseTag);

	// are we full auto?
	if (bFullAuto)
	{
		// schedule the next shot
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, RefireRate, false);
	} else {

		// for semi-auto weapons, schedule the cooldown notification
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::FireCooldownExpired, RefireRate, false);

	}
}

void AShooterWeapon::FireCooldownExpired()
{
	// notify the owner
	WeaponOwner->OnSemiWeaponRefire();
}

void AShooterWeapon::FinishReload()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsReloading = false;
	CurrentBullets = MagazineSize;

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
		WeaponOwner->OnWeaponReloadFinished();
	}
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
	if (!HasAuthority())
	{
		return;
	}

	// get the projectile transform
	FTransform ProjectileTransform = CalculateProjectileSpawnTransform(TargetLocation);
	
	// spawn the projectile
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = PawnOwner;

	AShooterProjectile* Projectile = GetWorld()->SpawnActor<AShooterProjectile>(ProjectileClass, ProjectileTransform, SpawnParams);

	TriggerMuzzleFlash();
	DrawShotImpactMarker(TargetLocation, ProjectileTransform);

	// play the firing montage
	WeaponOwner->PlayFiringMontage(FiringMontage);

	// add recoil
	WeaponOwner->AddWeaponRecoil(FiringRecoil);

	// consume bullets
	--CurrentBullets;

	// update the weapon HUD
	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
}

FTransform AShooterWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	// find the muzzle location
	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);

	// calculate the spawn location ahead of the muzzle
	const FVector SpawnLoc = MuzzleLoc + ((TargetLocation - MuzzleLoc).GetSafeNormal() * MuzzleOffset);

	// find the aim rotation vector while applying some variance to the target 
	const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetLocation + (UKismetMathLibrary::RandomUnitVector() * AimVariance));

	// return the built transform
	return FTransform(AimRot, SpawnLoc, FVector::OneVector);
}

void AShooterWeapon::TriggerMuzzleFlash()
{
	if (!MuzzleFlashLight || !GetWorld())
	{
		return;
	}

	MuzzleFlashLight->SetWorldLocation(FirstPersonMesh->GetSocketLocation(MuzzleSocketName));
	MuzzleFlashLight->SetIntensity(MuzzleFlashIntensity);
	MuzzleFlashLight->SetAttenuationRadius(MuzzleFlashRadius);
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
	MuzzleFlashLight->SetHiddenInGame(false);
	MuzzleFlashLight->SetVisibility(true);

	GetWorld()->GetTimerManager().SetTimer(MuzzleFlashTimer, this, &AShooterWeapon::HideMuzzleFlash, MuzzleFlashDuration, false);
}

void AShooterWeapon::HideMuzzleFlash()
{
	if (MuzzleFlashLight)
	{
		MuzzleFlashLight->SetVisibility(false);
		MuzzleFlashLight->SetHiddenInGame(true);
	}
}

void AShooterWeapon::DrawShotImpactMarker(const FVector& TargetLocation, const FTransform& ProjectileTransform) const
{
	if (!bDrawShotImpactMarker || !GetWorld())
	{
		return;
	}

	const FVector Start = ProjectileTransform.GetLocation();
	const FVector Direction = (TargetLocation - Start).GetSafeNormal();
	const FVector TraceEnd = TargetLocation + (Direction * 100.0f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	if (PawnOwner)
	{
		QueryParams.AddIgnoredActor(PawnOwner);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, TraceEnd, ECC_Visibility, QueryParams);
	const FVector MarkerLocation = bHit ? Hit.ImpactPoint + (Hit.ImpactNormal * 1.5f) : TargetLocation;

	DrawDebugSphere(GetWorld(), MarkerLocation, ShotImpactMarkerSize, 16, FColor::Red, false, ShotImpactMarkerLifeSpan, 0, 2.0f);
	DrawDebugPoint(GetWorld(), MarkerLocation, ShotImpactMarkerSize * 2.0f, FColor::Red, false, ShotImpactMarkerLifeSpan, 0);
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}
