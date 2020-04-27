// Fill out your copyright notice in the Description page of Project Settings.

#include "..\Public\SGrenadeLauncher.h"

ASGrenadeLauncher::ASGrenadeLauncher()
{

}

void ASGrenadeLauncher::BeginPlay()
{
	Super::BeginPlay();

	RateOfFire = 100;
	TimeBetweenShots = 60 / RateOfFire;

	MaxFireTypes = ProjectileClasses.Num() - 1;
	FireType = 0;

	MaxAmmo = 5;
	CurrentAmmo = MaxAmmo;
}

void ASGrenadeLauncher::StartFire()
{
	float FirstDelay = LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds;
	if (FirstDelay <= 0.0f)
		Fire();
}

void ASGrenadeLauncher::ToggleFireType()
{
	CurrentAmmo = 0;
	Super::ToggleFireType();
}

void ASGrenadeLauncher::Fire()
{
	if (CurrentAmmo <= 0) return;

	// Stop reload
	if (bIsReloading)
		StopReload();

	AActor* MyOwner = GetOwner();
	TSubclassOf<AActor> ProjectileClass = ProjectileClasses[FireType];
	if (MyOwner != nullptr && ProjectileClass != nullptr)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		//ActorSpawnParams.Instigator = MyOwner;

		// spawn the projectile at the muzzle
		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, ActorSpawnParams);

		LastFireTime = GetWorld()->TimeSeconds;

		--CurrentAmmo;
	}
}

FText ASGrenadeLauncher::GetCurrentFireTypeName()
{
	switch (FireType)
	{
	case 0: return FText::FromString("Normal");  break;
	case 1: return FText::FromString("Sticky"); break;
	case 2: return FText::FromString("Cluster"); break;
	default: return FText(); break;
	}
}