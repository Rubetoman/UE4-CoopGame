// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SExplosiveBarrel.h"
#include "../Public/Components/SHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	bExploded = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
}

// Called when the game starts or when spawned
void ASExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);
}

void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta, const class UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bExploded)
	{
		// Explode
		bExploded = true;

		// Change material
		MeshComp->SetMaterial(0, ExplodedMaterial);

		// Spawn explosion effect
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetTransform());

		//SetLifeSpan(10.0f);
	}
}