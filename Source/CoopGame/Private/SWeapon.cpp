// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "..\Public\SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include  "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame\CoopGame.h"
#include "TimerManager.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;
	VulnerableDamageMul = 4.0f;

	bIsReloading = false;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	RateOfFire = 600;
	TimeBetweenShots = 60 / RateOfFire;

	MaxFireTypes = 1;
	FireType = 0;

	MaxAmmo = 30;
	CurrentAmmo = MaxAmmo;

	ReloadTime = 2.0f;
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	if (FireType == 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
	}
	else if (FirstDelay <= 0.0f)
	{
		Fire();
	}	
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::StartReload()
{
	GetWorldTimerManager().SetTimer(TimerHandle_ReloadTime, this, &ASWeapon::Reload, ReloadTime);
	bIsReloading = true;
}

void ASWeapon::StopReload()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadTime);
	bIsReloading = false;
}

void ASWeapon::ToggleFireType()
{
	if (FireType == MaxFireTypes)
		FireType = 0;
	else
		++FireType;
}

FText ASWeapon::GetCurrentFireTypeName()
{
	switch (FireType)
	{
	case 0: return FText::FromString("Auto");  break;
	case 1: return FText::FromString("Semi-Auto"); break;
	default: return FText(); break;
	}
}

void ASWeapon::Fire()
{
	if (CurrentAmmo <= 0) return;

	// Stop reload
	if (bIsReloading)
		StopReload();

	// Trace the world. from pawn eyes to crosshair location
	AActor* MyOwner = GetOwner();

	if (MyOwner != nullptr)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// Smoke particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// Blocking hit! Process damage
			AActor* HitActor = Hit.GetActor();

			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
				ActualDamage *= VulnerableDamageMul;

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			UParticleSystem* SelectedEffect = nullptr;
			
			switch (SurfaceType)
			{
			case SURFACE_FLESHDEFAULT:
			case SURFACE_FLESHVULNERABLE:
				SelectedEffect = FleshImpactEffect;
				break;
			default:
				SelectedEffect = DefaultImpactEffect;
				break;
			}

			// Spawn hit effect
			if (SelectedEffect != nullptr)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

			TracerEndPoint = Hit.ImpactPoint;
		}

		if(DebugWeaponDrawing > 0)
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);

		PlayFireEffects(TracerEndPoint);

		LastFireTime = GetWorld()->TimeSeconds;

		--CurrentAmmo;
	}
}

void ASWeapon::Reload()
{
	CurrentAmmo = MaxAmmo;
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadTime);
	StopReload();
}

void ASWeapon::PlayFireEffects(FVector TracerEndPoint)
{
	// Spawn shot light particle effect
	if (MuzzleEffect != nullptr)
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);

	// Spawn smoke particle effect
	if (TracerEffect != nullptr)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if (TracerComp != nullptr)
			TracerComp->SetVectorParameter(TracerTargetName, TracerEndPoint);
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner != nullptr)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());

		if (PC != nullptr)
		{
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

