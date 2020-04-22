// Fill out your copyright notice in the Description page of Project Settings.


#include "SGrenadeLauncher.h"

void ASGrenadeLauncher::Fire()
{
	AActor* MyOwner = GetOwner();

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
	}
}