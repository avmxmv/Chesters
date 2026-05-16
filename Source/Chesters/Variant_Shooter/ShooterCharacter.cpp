// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "ShooterGameMode.h"
#include "Net/UnrealNetwork.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);

	static ConstructorHelpers::FClassFinder<AShooterWeapon> StartingWeaponBlueprint(TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol"));
	if (StartingWeaponBlueprint.Succeeded())
	{
		StartingWeaponClass = StartingWeaponBlueprint.Class;
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;
	DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	FirstPersonMeshBaseLocation = GetFirstPersonMesh()->GetRelativeLocation();
	FirstPersonMeshBaseRotation = GetFirstPersonMesh()->GetRelativeRotation();

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	TSubclassOf<AShooterWeapon> WeaponClassToSpawn = StartingWeaponClass;
	if (!WeaponClassToSpawn)
	{
		WeaponClassToSpawn = LoadClass<AShooterWeapon>(nullptr, TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol.BP_ShooterWeapon_Pistol_C"));
	}

	if (HasAuthority() && WeaponClassToSpawn)
	{
		AddWeaponClass(WeaponClassToSpawn);
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bEnableRunHandBob || !GetFirstPersonMesh())
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	if (Speed <= RunHandBobSpeedThreshold || GetCharacterMovement()->MaxWalkSpeed <= SlowWalkSpeed + KINDA_SMALL_NUMBER)
	{
		RunHandBobTime = 0.0f;
		GetFirstPersonMesh()->SetRelativeLocationAndRotation(FirstPersonMeshBaseLocation, FirstPersonMeshBaseRotation);
		return;
	}

	RunHandBobTime += DeltaSeconds * RunHandBobFrequency;

	const float BobSin = FMath::Sin(RunHandBobTime);
	const float BobCos = FMath::Cos(RunHandBobTime * 2.0f);
	const FVector BobLocation(
		RunHandBobLocationAmplitude.X * BobSin,
		RunHandBobLocationAmplitude.Y * BobSin,
		RunHandBobLocationAmplitude.Z * BobCos);
	const FRotator BobRotation(
		RunHandBobRotationAmplitude.Pitch * BobCos,
		RunHandBobRotationAmplitude.Yaw * BobSin,
		RunHandBobRotationAmplitude.Roll * BobSin);

	GetFirstPersonMesh()->SetRelativeLocationAndRotation(FirstPersonMeshBaseLocation + BobLocation, FirstPersonMeshBaseRotation + BobRotation);
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, OwnedWeapons);
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);
	}

	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AShooterCharacter::DoSelectWeaponSlot1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AShooterCharacter::DoSelectWeaponSlot2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AShooterCharacter::DoSelectWeaponSlot3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AShooterCharacter::DoSelectWeaponSlot4);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AShooterCharacter::DoStartSlowWalk);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AShooterCharacter::DoStopSlowWalk);
	PlayerInputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AShooterCharacter::DoStartSlowWalk);
	PlayerInputComponent->BindKey(EKeys::RightShift, IE_Released, this, &AShooterCharacter::DoStopSlowWalk);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AShooterCharacter::DoReload);
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Reduce HP
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	if (!HasAuthority())
	{
		ServerStartFiring();
		return;
	}

	// fire the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StartFiring();
	}
}

void AShooterCharacter::DoStopFiring()
{
	if (!HasAuthority())
	{
		ServerStopFiring();
		return;
	}

	// stop firing the current weapon
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	if (!HasAuthority())
	{
		ServerSwitchWeapon();
		return;
	}

	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && CurrentWeapon && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.IndexOfByPredicate([this](const TObjectPtr<AShooterWeapon>& Weapon)
		{
			return Weapon == CurrentWeapon;
		});

		if (WeaponIndex == INDEX_NONE)
		{
			return;
		}

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon();
		RefreshWeaponAttachments();
	}
}

void AShooterCharacter::DoSelectWeaponSlot(int32 WeaponIndex)
{
	if (!HasAuthority())
	{
		ServerSelectWeaponSlot(WeaponIndex);
		return;
	}

	if (!OwnedWeapons.IsValidIndex(WeaponIndex) || !CurrentWeapon || IsDead())
	{
		return;
	}

	AShooterWeapon* SelectedWeapon = OwnedWeapons[WeaponIndex];
	if (!SelectedWeapon || SelectedWeapon == CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->DeactivateWeapon();
	CurrentWeapon = SelectedWeapon;
	CurrentWeapon->ActivateWeapon();
	RefreshWeaponAttachments();
}

void AShooterCharacter::DoSelectWeaponSlot1()
{
	DoSelectWeaponSlot(0);
}

void AShooterCharacter::DoSelectWeaponSlot2()
{
	DoSelectWeaponSlot(1);
}

void AShooterCharacter::DoSelectWeaponSlot3()
{
	DoSelectWeaponSlot(2);
}

void AShooterCharacter::DoSelectWeaponSlot4()
{
	DoSelectWeaponSlot(3);
}

void AShooterCharacter::DoStartSlowWalk()
{
	if (IsDead())
	{
		return;
	}

	ApplySlowWalk(true);

	if (!HasAuthority())
	{
		ServerStartSlowWalk();
	}
}

void AShooterCharacter::DoStopSlowWalk()
{
	ApplySlowWalk(false);

	if (!HasAuthority())
	{
		ServerStopSlowWalk();
	}
}

void AShooterCharacter::DoReload()
{
	if (!HasAuthority())
	{
		if (CurrentWeapon && !IsDead() && CurrentWeapon->CanReload())
		{
			OnReloadStarted.Broadcast(CurrentWeapon->GetReloadDuration());
		}

		ServerReload();
		return;
	}

	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->Reload();
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, ThirdPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	UAnimMontage* MontageToPlay = Montage;
	if (!MontageToPlay)
	{
		MontageToPlay = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage.MM_Pistol_Fire_Montage"));
	}

	if (!MontageToPlay)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetFirstPersonMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(MontageToPlay);
	}
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	if (!HasAuthority() || !WeaponClass)
	{
		return;
	}

	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
			RefreshWeaponAttachments();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	GetFirstPersonMesh()->SetHiddenInGame(false, true);
	GetFirstPersonMesh()->SetVisibility(true, true);
	Weapon->GetFirstPersonMesh()->SetHiddenInGame(false, true);
	Weapon->GetFirstPersonMesh()->SetVisibility(true, true);

	// set the character mesh AnimInstances
	if (Weapon->GetFirstPersonAnimInstanceClass())
	{
		GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	}
	else
	{
		static TSubclassOf<UAnimInstance> DefaultPistolAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Variant_Shooter/Anims/ABP_FP_Pistol.ABP_FP_Pistol_C"));
		if (DefaultPistolAnimClass)
		{
			GetFirstPersonMesh()->SetAnimInstanceClass(DefaultPistolAnimClass);
		}
	}

	if (Weapon->GetThirdPersonAnimInstanceClass())
	{
		GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
	}
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

void AShooterCharacter::OnWeaponReloadStarted(float ReloadDuration)
{
	OnReloadStarted.Broadcast(ReloadDuration);
}

void AShooterCharacter::OnWeaponReloadFinished()
{
	OnReloadFinished.Broadcast();
}

void AShooterCharacter::OnRep_CurrentWeapon()
{
	RefreshWeaponAttachments();

	if (CurrentWeapon)
	{
		OnWeaponActivated(CurrentWeapon);
	}
	else
	{
		OnBulletCountUpdated.Broadcast(0, 0);
	}
}

void AShooterCharacter::OnRep_OwnedWeapons()
{
	RefreshWeaponAttachments();
}

void AShooterCharacter::RefreshWeaponAttachments()
{
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (!Weapon)
		{
			continue;
		}

		AttachWeaponMeshes(Weapon);
		Weapon->SetActorHiddenInGame(Weapon != CurrentWeapon);
	}
}

void AShooterCharacter::ServerStartFiring_Implementation()
{
	DoStartFiring();
}

void AShooterCharacter::ServerStopFiring_Implementation()
{
	DoStopFiring();
}

void AShooterCharacter::ServerSwitchWeapon_Implementation()
{
	DoSwitchWeapon();
}

void AShooterCharacter::ServerSelectWeaponSlot_Implementation(int32 WeaponIndex)
{
	DoSelectWeaponSlot(WeaponIndex);
}

void AShooterCharacter::ServerStartSlowWalk_Implementation()
{
	if (!IsDead())
	{
		ApplySlowWalk(true);
	}
}

void AShooterCharacter::ServerStopSlowWalk_Implementation()
{
	ApplySlowWalk(false);
}

void AShooterCharacter::ServerReload_Implementation()
{
	DoReload();
}

void AShooterCharacter::ApplySlowWalk(bool bSlowWalk)
{
	if (!GetCharacterMovement())
	{
		return;
	}

	if (DefaultWalkSpeed <= 0.0f)
	{
		DefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	GetCharacterMovement()->MaxWalkSpeed = bSlowWalk ? SlowWalkSpeed : DefaultWalkSpeed;
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon && Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

bool AShooterCharacter::OwnsWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	return FindWeaponOfType(WeaponClass) != nullptr;
}
