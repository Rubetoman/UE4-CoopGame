// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UAudioComponent;
class UDamageType;
class USoundCue;

// Contains information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

	//////////////////////////////////////////////////////////////////////////
	// Weapon Input
	virtual void StartFire();
	void StopFire();

	void StartReload();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartReload();

	void StopReload();

	virtual void ToggleFireType();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual FText GetCurrentFireTypeName();

public:
	UPROPERTY(Replicated, VisibleDefaultsOnly, Category = "Weapon")
	bool bIsReloading;

	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Powerup")
	bool bExplosiveBullets;

	bool bInAimingMode;

protected:
	virtual void BeginPlay() override;

	//////////////////////////////////////////////////////////////////////////
	// Weapon Input
	virtual void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	void Reload();

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	/** Helper for playing sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	void PlayReloadSound();

	//////////////////////////////////////////////////////////////////////////
	// Replication & effects
	void PlayFireEffects(FVector TracerEndPoint);

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	UFUNCTION()
	void OnRep_HitScanTrace();


protected:
	float LastFireTime;

	// RPM - Bullets per minute fired by weapon
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	// Bullet Spread in Degrees
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpreadMin;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpreadMax;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpreadRate;

	UPROPERTY(VisibleDefaultsOnly, Category = "Weapon")
	float BulletSpread;

	int32 ShotNumber;

	// Derived from RateOfFire
	float TimeBetweenShots;

	// Change between fire types. Default: 0 = Automatic, 1 = Semi-Automatic
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 FireType;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	uint8 MaxFireTypes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Powerup")
	UParticleSystem* ExplosionEffect = nullptr;

	/** Explosion sound (when explosive power up is active)*/
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* ExplosionSound = nullptr;

	/** Looped fire sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireSound = nullptr;

	/** End fire sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireFinishSound = nullptr;

	/** Empty magazine fire sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* FireEmptySound = nullptr;

	/** Reload sound */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundCue* ReloadSound = nullptr;

	/** Offset to play reload sound (at the start and end of the reload) */
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	float ReloadSoundOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float VulnerableDamageMul;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	uint8 MaxAmmo;

	UPROPERTY(Replicated, VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	uint8 CurrentAmmo;

	// Timer Handles
	FTimerHandle TimerHandle_TimeBetweenShots;
	FTimerHandle TimerHandle_ReloadTime;
	FTimerHandle TimerHandle_FirstReloadSoundTime;
	FTimerHandle TimerHandle_SecondReloadSoundTime;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ReloadTime;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UPROPERTY(Transient)
	UAudioComponent* FireAC;
};
