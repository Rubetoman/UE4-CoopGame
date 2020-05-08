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
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Net\UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	if (GetLocalRole() == ROLE_Authority)
	{
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
			CurrentWeapon.Index = 0;
			CurrentWeapon.Weapon = PlayerWeapons[0];
			CurrentWeapon.Weapon->SetActorHiddenInGame(false);
		}
	}

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
	if (CurrentWeapon.Weapon != nullptr && !bIsChangingWeapon)
		CurrentWeapon.Weapon->StartFire();
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon.Weapon != nullptr)
		CurrentWeapon.Weapon->StopFire();
}

void ASCharacter::Reload()
{
	if (CurrentWeapon.Weapon != nullptr && !CurrentWeapon.Weapon->bIsReloading)
		CurrentWeapon.Weapon->StartReload();
}

void ASCharacter::ToggleFireType()
{
	if (CurrentWeapon.Weapon != nullptr)
	{
		CurrentWeapon.Weapon->ToggleFireType();
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
	CurrentWeapon.Weapon->SetActorHiddenInGame(true);

	if (CurrentWeapon.Index < PlayerWeapons.Num() - 1)
		++CurrentWeapon.Index;
	else
		CurrentWeapon.Index = 0;

	CurrentWeapon.Weapon = PlayerWeapons[CurrentWeapon.Index];
	CurrentWeapon.Weapon->SetActorHiddenInGame(false);

	LastChangeTime = GetWorld()->TimeSeconds;

	EndEquipWeapon();
}

void ASCharacter::StartPreviousWeapon()
{
	bIsChangingWeapon = true;
	float FirstDelay = FMath::Max(LastChangeTime + WeaponChangeTime - GetWorld()->TimeSeconds, 0.0f);
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponChangeTime, this, &ASCharacter::PreviousWeapon, WeaponChangeTime, true, FirstDelay);
}

void ASCharacter::PreviousWeapon()
{
	CurrentWeapon.Weapon->SetActorHiddenInGame(true);

	if (CurrentWeapon.Index > 0)
		--CurrentWeapon.Index;
	else
		CurrentWeapon.Index = PlayerWeapons.Num() - 1;

	CurrentWeapon.Weapon = PlayerWeapons[CurrentWeapon.Index];
	CurrentWeapon.Weapon->SetActorHiddenInGame(false);

	LastChangeTime = GetWorld()->TimeSeconds;

	EndEquipWeapon();
}

void ASCharacter::StartEquipWeapon(uint8 WeaponIndex)
{
	bIsChangingWeapon = true;
	float FirstDelay = FMath::Max(LastChangeTime + WeaponChangeTime - GetWorld()->TimeSeconds, 0.0f);
	FTimerDelegate EquipWeaponDelegate = FTimerDelegate::CreateUObject(this, &ASCharacter::EquipWeapon, WeaponIndex);
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponChangeTime, EquipWeaponDelegate, WeaponChangeTime, true, FirstDelay);
}

void ASCharacter::EquipWeapon(uint8 WeaponIndex)
{
	if (WeaponIndex < PlayerWeapons.Num())
	{
		ASWeapon* NewWeapon = PlayerWeapons[WeaponIndex];
		if (NewWeapon != nullptr && NewWeapon != CurrentWeapon.Weapon)
		{
			CurrentWeapon.Weapon->SetActorHiddenInGame(true);
			CurrentWeapon.Index = WeaponIndex;
			CurrentWeapon.Weapon = NewWeapon;
			LastChangeTime = GetWorld()->TimeSeconds;
			CurrentWeapon.Weapon->SetActorHiddenInGame(false);
		}
	}
	EndEquipWeapon();
}

void ASCharacter::EndEquipWeapon()
{
	OnWeaponChange();
	GetWorldTimerManager().ClearTimer(TimerHandle_WeaponChangeTime);
	bIsChangingWeapon = false;
}

void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
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
	return CurrentWeapon.Weapon;
}

bool ASCharacter::GetIsReloading()
{
	if (CurrentWeapon.Weapon != nullptr)
		return CurrentWeapon.Weapon->bIsReloading;

	return false;
}

void ASCharacter::ChangeMaxWalkSpeed(float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ASCharacter::ClientChangeMaxWalkSpeed_Implementation(float NewSpeed)
{
	if (GetLocalRole() != ROLE_Authority)
		ChangeMaxWalkSpeed(NewSpeed);
}

bool ASCharacter::ClientChangeMaxWalkSpeed_Validate(float NewSpeed)
{
	return true;
}

void ASCharacter::SetExplosiveBullets(bool bExplosive)
{
	if (CurrentWeapon.Weapon != nullptr)
		CurrentWeapon.Weapon->bExplosiveBullets = bExplosive;
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

	/* Weapon 1 to 9*/
	for(int i = 1; i < LAST_WEAPON_KEY; ++i)
	{
		// Create delegate to bind and set the function
		FInputActionHandlerSignature ActionHandler;
		ActionHandler.BindUFunction(this, TEXT("StartEquipWeapon"), i - 1);

		// Set the input binding
		FString InputName("Weapon");
		InputName += FString::FromInt(i);
		//FName InputName = InputNameStr;
		FInputActionBinding* ActionBinding = new FInputActionBinding(FName(*InputName), IE_Pressed);
		ActionBinding->ActionDelegate = ActionHandler;
		PlayerInputComponent->AddActionBinding(*ActionBinding);
	}
}

FVector ASCharacter::GetPawnViewLocation() const
{
	if(CameraComp != nullptr)
		return CameraComp->GetComponentLocation();

	return Super::GetPawnViewLocation();
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, PlayerWeapons);
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bIsChangingWeapon);
}