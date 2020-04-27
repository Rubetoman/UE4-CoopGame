// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"
#include "..\Public\SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "..\Public\SWeapon.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame\CoopGame.h"
#include "../Public/Components/SHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
//#include "Containers/Map.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetMovementComponent()->GetNavAgentPropertiesRef().bCanJump = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	WeaponAttachSocketName = "WeaponSocket";

	WeaponChangeTime = 1.0f;

	bDied = false; 
	bIsChangingWeapon = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;
	TargetFOV = DefaultFOV;

	// Spawn a default weapon
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (TSubclassOf<ASWeapon> WeaponClass : StarterWeaponClasses)
	{		
		ASWeapon* Weapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (Weapon != nullptr)
		{
			Weapon->SetOwner(this);
			Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
			Weapon->SetActorHiddenInGame(true);
			PlayerWeapons.Add(Weapon);
		}
	}

	// Set current weapon
	if (PlayerWeapons[0] != nullptr)
	{
		CurrentWeapon.Key = 0;
		CurrentWeapon.Value = PlayerWeapons[0];
		CurrentWeapon.Value->SetActorHiddenInGame(false);

	}

	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	OnCharacterStart();
}

void ASCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

void ASCharacter::BeginZoom()
{
	TargetFOV = ZoomedFOV;
}

void ASCharacter::EndZoom()
{
	TargetFOV = DefaultFOV;
}

void ASCharacter::StartFire()
{
	if (CurrentWeapon.Value != nullptr && !bIsChangingWeapon)
		CurrentWeapon.Value->StartFire();
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon.Value != nullptr)
		CurrentWeapon.Value->StopFire();
}

void ASCharacter::Reload()
{
	if (CurrentWeapon.Value != nullptr && !CurrentWeapon.Value->bIsReloading)
		CurrentWeapon.Value->StartReload();
}

void ASCharacter::ToggleFireType()
{
	if (CurrentWeapon.Value != nullptr)
	{
		CurrentWeapon.Value->ToggleFireType();
		OnToggleFireType();	// Blueprint implemented
	}
}

void ASCharacter::StartNextWeapon()
{
	bIsChangingWeapon = true;
	float FirstDelay = FMath::Max(LastChangeTime + WeaponChangeTime - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponChangeTime, this, &ASCharacter::PreviousWeapon, WeaponChangeTime, true, FirstDelay);
}

void ASCharacter::NextWeapon()
{
	CurrentWeapon.Value->SetActorHiddenInGame(true);

	if (CurrentWeapon.Key < PlayerWeapons.Num() - 1)
		++CurrentWeapon.Key;
	else
		CurrentWeapon.Key = 0;

	CurrentWeapon.Value = PlayerWeapons[CurrentWeapon.Key];
	CurrentWeapon.Value->SetActorHiddenInGame(false);

	LastChangeTime = GetWorld()->TimeSeconds;

	EndNextWeapon();
}

void ASCharacter::EndNextWeapon()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_WeaponChangeTime);
	bIsChangingWeapon = false;
}

void ASCharacter::StartPreviousWeapon()
{
	bIsChangingWeapon = true;
	float FirstDelay = FMath::Max(LastChangeTime + WeaponChangeTime - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponChangeTime, this, &ASCharacter::PreviousWeapon, WeaponChangeTime, true, FirstDelay);
}

void ASCharacter::PreviousWeapon()
{
	CurrentWeapon.Value->SetActorHiddenInGame(true);

	if (CurrentWeapon.Key > 0)
		--CurrentWeapon.Key;
	else
		CurrentWeapon.Key = PlayerWeapons.Num() - 1;

	CurrentWeapon.Value = PlayerWeapons[CurrentWeapon.Key];
	CurrentWeapon.Value->SetActorHiddenInGame(false);

	LastChangeTime = GetWorld()->TimeSeconds;

	EndPreviousWeapon();
}

void ASCharacter::EndPreviousWeapon()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_WeaponChangeTime);
	bIsChangingWeapon = false;
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType, 
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bDied)
	{
		// Die
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); 

		DetachFromControllerPendingDestroy();

		SetLifeSpan(10.0f);
	}
}

ASWeapon* ASCharacter::GetCurrentWeapon()
{
	return CurrentWeapon.Value;
}

bool ASCharacter::GetIsReloading()
{
	if (CurrentWeapon.Value != nullptr)
		return CurrentWeapon.Value->bIsReloading;

	return false;
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetFOV != CameraComp->FieldOfView)
	{
		float newFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
		CameraComp->SetFieldOfView(newFOV);
	}
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);

	PlayerInputComponent->BindAction("ToggleFireType", IE_Pressed, this, &ASCharacter::	ToggleFireType);

	PlayerInputComponent->BindAction("NextWeapon", IE_Pressed, this, &ASCharacter::StartNextWeapon);
	PlayerInputComponent->BindAction("PreviousWeapon", IE_Pressed, this, &ASCharacter::StartPreviousWeapon);

}

FVector ASCharacter::GetPawnViewLocation() const
{
	if(CameraComp != nullptr)
		return CameraComp->GetComponentLocation();

	return Super::GetPawnViewLocation();
}

