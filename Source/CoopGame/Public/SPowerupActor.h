// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerupActor.generated.h"

UCLASS()
class COOPGAME_API ASPowerupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerupActor();

protected:
	UFUNCTION()
	void OnTickPowerup();

	UFUNCTION()
	void OnRep_PowerupActive();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerupStateChanged(bool bNewIsActive);

	// Time between powerup ticks
	UPROPERTY(EditDefaultsOnly, Category = "Powerups")
	float PowerupInterval;

	// Total times we apply the powerup effect
	UPROPERTY(EditDefaultsOnly, Category = "Powerups")
	int32 TotalNrOfTicks;

	// Total number of ticks applied
	FTimerHandle TimerHandle_PowerupTick;

	int32 TicksProcessed;

	// Keeps state of the power-up
	UPROPERTY(ReplicatedUsing=OnRep_PowerupActive)
	bool bIsPowerupActive;

public:

	void ActivatePowerup(AActor* OtherActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnActivated(AActor* OtherActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnPowerUpTicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Powerups")
	void OnExpired();

};
