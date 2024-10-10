// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactoryVolume.h"
#include "BasePaperGolfVolumeActorFactory.generated.h"

/**
 * 
 */
UCLASS(MinimalAPI, config = Editor, collapsecategories, hidecategories = Object)
class UBasePaperGolfVolumeActorFactory : public UActorFactoryVolume
{
	GENERATED_BODY()

public:
	UBasePaperGolfVolumeActorFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorFactory Interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	//~ End UActorFactory Interface
	
};
