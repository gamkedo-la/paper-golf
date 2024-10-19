// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Tutorial/MessageOrientedTutorialAction.h"
#include "ShotPreviewTutorialAction.generated.h"

/**
 * 
 */
UCLASS()
class UShotPreviewTutorialAction : public UMessageOrientedTutorialAction
{
	GENERATED_BODY()

public:
	UShotPreviewTutorialAction();
	virtual bool IsRelevant() const override;
	
};
