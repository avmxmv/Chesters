// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeaponInfoTypes.generated.h"

class AShooterWeapon;
class UTexture2D;

USTRUCT(BlueprintType)
struct FShooterWeaponDisplayOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Weapons")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Weapons")
	TSubclassOf<AShooterWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shooter|Weapons")
	TObjectPtr<UTexture2D> Icon = nullptr;
};

USTRUCT(BlueprintType)
struct FShooterWeaponInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	TSubclassOf<AShooterWeapon> WeaponClass;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	float FireRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	float ReloadDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	bool bExplosive = false;

	UPROPERTY(BlueprintReadOnly, Category="Shooter|Weapons")
	float ExplosionRadius = 0.0f;
};
