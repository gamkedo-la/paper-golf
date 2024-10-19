// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TutorialConfigDataAsset.generated.h"

class UInputAction;

/**
 * 
 */
UCLASS()
class PGUI_API UTutorialConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Input", EditDefaultsOnly)
	TObjectPtr<UInputAction> SkipTutorialAction{};
};
