// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SWeapon.h"
#include "SGrenadeLauncher.generated.h"

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGrenadeLauncher : public ASWeapon
{
	GENERATED_BODY()

public:
	ASGrenadeLauncher();

	virtual void BeginPlay() override;

	virtual void StartFire() override;
	void ToggleFireType() override;

	FText GetCurrentFireTypeName() override;
	
protected:

	virtual void Fire() override;

	/* Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile Weapon")
	TArray<TSubclassOf<AActor>> ProjectileClasses;
};
