// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Tutorial/MessageOrientedTutorialAction.h"
#include "AimTutorialAction.generated.h"

/**
 * 
 */
UCLASS()
class UAimTutorialAction : public UMessageOrientedTutorialAction
{
	GENERATED_BODY()
	
public:
	UAimTutorialAction();
	virtual bool IsRelevant() const override;
};
