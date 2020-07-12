// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "..\Public\SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components\AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include  "PhysicalMaterials/PhysicalMaterial.h"
#include "CoopGame\CoopGame.h"
#include "TimerManager.h"
#include "Net\UnrealNetwork.h"
#include "Sound\SoundCue.h"

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

	RateOfFire = 300.0f;
	BulletSpread = 2.0f;
	BulletSpreadMin = 2.0f;
	BulletSpreadMax = 2.0f;
	BulletSpreadRate = 0.2f;

	ShotNumber = 0;

	bIsReloading = false;

	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	MaxFireTypes = 1;
	FireType = 0;

	MaxAmmo = 30;
	CurrentAmmo = MaxAmmo;

	ReloadTime = 2.0f;
	ReloadSoundOffset = 0.3f;

	bExplosiveBullets = false;
	bInAimingMode = false;

	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::StartFire()
{
	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);
	if (FirstDelay <= 0.0f)
	{
		// Reset bullet spread
		BulletSpread = BulletSpreadMin;
		ShotNumber = 0;
	}

	if (FireType == 0)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);

		// Play looped fire sound
		if (FireAC == NULL && CurrentAmmo > 0)
		{
			FireAC = PlayWeaponSound(LoopedFireSound);
		}
	}
	else if (FirstDelay <= 0.0f)
	{
		Fire();

		// Play single fire sound
		if (CurrentAmmo > 0)
			PlayWeaponSound(SingleFireSound);
	}	
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);

	if (FireAC != nullptr)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = nullptr;

		PlayWeaponSound(FireFinishSound);
	}
}

void ASWeapon::StartReload()
{
	if (CurrentAmmo < MaxAmmo)
	{
		// Networking
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerStartReload();
		}

		GetWorldTimerManager().SetTimer(TimerHandle_ReloadTime, this, &ASWeapon::Reload, ReloadTime);

		// Play reload sounds with offset
		GetWorldTimerManager().SetTimer(TimerHandle_FirstReloadSoundTime, this, &ASWeapon::PlayReloadSound, ReloadSoundOffset);
		GetWorldTimerManager().SetTimer(TimerHandle_SecondReloadSoundTime, this, &ASWeapon::PlayReloadSound, ReloadTime - ReloadSoundOffset);

		bIsReloading = true;
	}
}

void ASWeapon::StopReload()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadTime);
	GetWorldTimerManager().ClearTimer(TimerHandle_FirstReloadSoundTime);
	GetWorldTimerManager().ClearTimer(TimerHandle_SecondReloadSoundTime);
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
	if (CurrentAmmo <= 0)
	{
		StopFire();
		PlayWeaponSound(FireEmptySound);
		return;
	}
	// Stop reload
	if (bIsReloading)
		StopReload();

	// Networking
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerFire();
	}

	// Trace the world. from pawn eyes to crosshair location
	AActor* MyOwner = GetOwner();

	if (MyOwner != nullptr)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		// Bullet Spread
		float HalfRad = FMath::DegreesToRadians(bInAimingMode? BulletSpread * 0.4 : BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		// Add to bullet spread from automatic fire.
		if (BulletSpreadRate > 0 && BulletSpread < BulletSpreadMax && ShotNumber > 0)
		{
			BulletSpread = BulletSpread + BulletSpreadRate;
		}
		++ShotNumber;

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// Smoke particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// Blocking hit! Process damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *= VulnerableDamageMul;
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), CriticalHitSound, Hit.Location);
			}
			else
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), NormalHitSound, Hit.Location);
			}
			
			if (bExplosiveBullets)
				ActualDamage *= 2.0f;

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);
			DrawDebugPoint(GetWorld(), TracerEndPoint, 5.0f, FColor::Red, false, 10.0, 5.0f);
		}

		PlayFireEffects(TracerEndPoint);

		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFireTime = GetWorld()->TimeSeconds;

		--CurrentAmmo;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::Reload()
{
	CurrentAmmo = MaxAmmo;
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadTime);
	StopReload();
}

void ASWeapon::PlayReloadSound()
{
	PlayWeaponSound(ReloadSound);
}

void ASWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool ASWeapon::ServerStartReload_Validate()
{
	return true;
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

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
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
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());

		if (bExplosiveBullets && ExplosionEffect != nullptr)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, ImpactPoint, ShotDirection.Rotation());
			UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
		}
	}

}

UAudioComponent* ASWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	AActor* MyOwner = GetOwner();
	if (Sound != nullptr && MyOwner != nullptr)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyOwner->GetRootComponent());
	}

	return AC;
}

void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);

	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME(ASWeapon, CurrentAmmo);
	DOREPLIFETIME(ASWeapon, bIsReloading);
	DOREPLIFETIME(ASWeapon, bExplosiveBullets);
}