// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Tutorial/TutorialAction.h"
#include "AimTutorialAction.generated.h"

/**
 * 
 */
UCLASS()
class UAimTutorialAction : public UTutorialAction
{
	GENERATED_BODY()
	
public:
	virtual bool IsRelevant() const override;
	virtual void Execute() override;
};
