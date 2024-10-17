// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "Tutorial/TutorialAction.h"
#include "ShotTutorialAction.generated.h"

/**
 * 
 */
UCLASS()
class UShotTutorialAction : public UTutorialAction
{
	GENERATED_BODY()
	
public:
	virtual void Execute() override;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	float MessageDuration{ 1.5f };
};
