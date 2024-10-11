// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/WaterHazardBoundsVolume.h"

#include "Components/StaticMeshComponent.h"

#include "Utils/PGAudioUtilities.h"

#include "Pawn/PaperGolfPawn.h"

#include "Utils/CollisionUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WaterHazardBoundsVolume)

AWaterHazardBoundsVolume::AWaterHazardBoundsVolume()
{
	HazardType = EHazardType::Water;
	Type = EPaperGolfVolumeOverlapType::Any;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(PG::CollisionProfile::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);

	WaterTableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water Table"));
	WaterTableMesh->SetMobility(EComponentMobility::Static);
	WaterTableMesh->SetupAttachment(RootComponent);
	WaterTableMesh->SetCollisionProfileName("BlockAll");
	Mesh->SetGenerateOverlapEvents(false);
}
