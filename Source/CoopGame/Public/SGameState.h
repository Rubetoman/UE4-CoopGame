// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SGameState.generated.h"

UENUM(BluePrintType)
enum class EWaveState : uint8
{
	PreparingNextWave,
	WaveInProgress,
	WaitingToComplete,	// No longer spawning bots, waiting for players to kill remaining bots
	WaveComplete,
	GameOver,
};

/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameState : public AGameStateBase
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void OnRep_WaveState(EWaveState OldState);

	UFUNCTION(BlueprintImplementableEvent, Category = "GameState")
	void WaveStateChanged(EWaveState NewState, EWaveState OldState);
	
public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WaveState, Category = "GameState")
	EWaveState WaveState;
};
