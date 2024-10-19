// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Tutorial/MessageOrientedTutorialAction.h"
#include "ShotSpinTutorialAction.generated.h"

/**
 * 
 */
UCLASS()
class UShotSpinTutorialAction : public UMessageOrientedTutorialAction
{
	GENERATED_BODY()

public:
	UShotSpinTutorialAction();
	virtual bool IsRelevant() const override;
};
