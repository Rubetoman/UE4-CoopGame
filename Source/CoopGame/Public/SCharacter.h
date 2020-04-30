// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;

USTRUCT()
struct FWeaponStruct
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Index;		// Index of the weapon on player weapons list

	UPROPERTY()
	ASWeapon* Weapon = nullptr;	// Pointer to the weapon

};

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Player")
	void OnCharacterStart();

	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginCrouch();
	void EndCrouch();

	void BeginZoom();
	void EndZoom();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartFire();
	void StopFire();

	void Reload();

	void ToggleFireType();

	void StartNextWeapon();
	void NextWeapon();

	void StartPreviousWeapon();
	void PreviousWeapon();
	
	UFUNCTION()
	void StartEquipWeapon(uint8 WeaponIndex);
	void EquipWeapon(uint8 WeaponIndex);

	void EndEquipWeapon();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player")
	void OnWeaponChange();

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnToggleFireType();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ASWeapon* GetCurrentWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool GetIsReloading();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp = nullptr;

	float TargetFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float ZoomedFOV = 65.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Player", meta =(ClampMin = 0.1f, ClampMax = 100.0f))
	float ZoomInterpSpeed = 20.0f;

	// Default FOV set during begin play
	float DefaultFOV;

	UPROPERTY(Replicated)
	FWeaponStruct CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TArray<TSubclassOf<ASWeapon>> StarterWeaponClasses;

	UPROPERTY(Replicated, VisibleDefaultsOnly, Category = "Player")
	TArray<ASWeapon*> PlayerWeapons;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

	// Pawn is dead?
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bDied;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsChangingWeapon;

	FTimerHandle TimerHandle_WeaponChangeTime;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float WeaponChangeTime;

	float LastChangeTime;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

};
