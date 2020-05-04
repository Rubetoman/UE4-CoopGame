// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SExplosiveBarrel.h"
#include "../Public/Components/SHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Net\UnrealNetwork.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	bExploded = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bIgnoreOwningActor = true;

	JumpImpulse = 500;

	SetReplicates(true);
	SetReplicateMovement(true);
}

void ASExplosiveBarrel::ExplodeEffects()
{
	// Change material
	MeshComp->SetMaterial(0, ExplodedMaterial);

	// Spawn explosion effect
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
}

void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bExploded)
	{
		// Explode
		bExploded = true;
		OnRep_Exploded();
		ExplodeEffects();

		// Add force to barrel
		MeshComp->AddImpulse(FVector::UpVector * JumpImpulse, NAME_None, true);

		// Add radial force
		RadialForceComp->FireImpulse();
	}
}

void ASExplosiveBarrel::OnRep_Exploded()
{
	ExplodeEffects();
}

void ASExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASExplosiveBarrel, bExploded);
}