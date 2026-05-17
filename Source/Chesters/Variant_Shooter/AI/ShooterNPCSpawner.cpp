// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterNPCSpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "ShooterNPC.h"
#include "ShooterGameMode.h"

// Sets default values
AShooterNPCSpawner::AShooterNPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the reference spawn capsule
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

void AShooterNPCSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	InitialSpawnCount = SpawnCount;

	if (Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		return;
	}

	// ensure we don't spawn NPCs if our initial spawn count is zero
	if (SpawnCount > 0)
	{
		// schedule the first NPC spawn
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterNPCSpawner::SpawnNPC, InitialSpawnDelay);
	}
}

void AShooterNPCSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the spawn timer
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}

void AShooterNPCSpawner::ResetForRound(float SpawnDelay)
{
	if (InitialSpawnCount <= 0)
	{
		InitialSpawnCount = SpawnCount;
	}

	StopSpawning();

	SpawnCount = InitialSpawnCount;
	if (SpawnCount <= 0)
	{
		return;
	}

	if (SpawnDelay > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterNPCSpawner::SpawnNPC, SpawnDelay, false);
	}
	else
	{
		SpawnNPC();
	}
}

void AShooterNPCSpawner::StopSpawning()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	if (IsValid(SpawnedNPC))
	{
		SpawnedNPC->Destroy();
	}

	SpawnedNPC = nullptr;
}

void AShooterNPCSpawner::SpawnNPC()
{
	if (SpawnCount <= 0)
	{
		return;
	}

	// ensure the NPC class is valid
	if (IsValid(NPCClass))
	{
		// spawn the NPC at the reference capsule's transform
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		SpawnedNPC = GetWorld()->SpawnActor<AShooterNPC>(NPCClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

		// was the NPC successfully created?
		if (SpawnedNPC)
		{
			// subscribe to the death delegate
			SpawnedNPC->OnPawnDeath.AddDynamic(this, &AShooterNPCSpawner::OnNPCDied);
		}
	}
}

void AShooterNPCSpawner::OnNPCDied()
{
	SpawnedNPC = nullptr;

	// decrease the spawn counter
	--SpawnCount;

	// is this the last NPC we should spawn?
	if (SpawnCount <= 0)
	{
		return;
	}

	// schedule the next NPC spawn
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterNPCSpawner::SpawnNPC, RespawnDelay);
}
