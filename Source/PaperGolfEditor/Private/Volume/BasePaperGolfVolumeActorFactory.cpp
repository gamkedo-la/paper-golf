// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Volume/BasePaperGolfVolumeActorFactory.h"

#include "Volume/HazardBoundsVolume.h"
#include "Builders/CubeBuilder.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasePaperGolfVolumeActorFactory)

UBasePaperGolfVolumeActorFactory::UBasePaperGolfVolumeActorFactory(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	DisplayName = FText::FromString(TEXT("Hazard Bounds Volume"));
	NewActorClass = AHazardBoundsVolume::StaticClass();
}

bool UBasePaperGolfVolumeActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (UActorFactory::CanCreateActorFrom(AssetData, OutErrorMsg))
	{
		return true;
	}

	if (AssetData.IsValid() && !AssetData.IsInstanceOf(AHazardBoundsVolume::StaticClass()))
	{
		return false;
	}

	return true;
}

void UBasePaperGolfVolumeActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AHazardBoundsVolume* VolumeActor = CastChecked<AHazardBoundsVolume>(NewActor);
	if (VolumeActor != nullptr)
	{
		UCubeBuilder* Builder = NewObject<UCubeBuilder>();
		CreateBrushForVolumeActor(VolumeActor, Builder);
	}
}
