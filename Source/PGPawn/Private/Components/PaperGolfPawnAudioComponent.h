// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/PGHitSfxComponent.h"
#include "PaperGolfPawnAudioComponent.generated.h"

/**
 * 
 */
UCLASS()
class UPaperGolfPawnAudioComponent : public UPGHitSfxComponent
{
	GENERATED_BODY()
	
public:
	UPaperGolfPawnAudioComponent();

	void PlayFlick();
	void PlayTurnStart(); 

private:
};
