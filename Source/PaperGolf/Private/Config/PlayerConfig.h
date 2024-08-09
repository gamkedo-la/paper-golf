// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "PlayerConfig.generated.h"


/**
 * 
 */
UCLASS()
class UPlayerConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TArray<FLinearColor> AvailablePlayerColors{};
};
