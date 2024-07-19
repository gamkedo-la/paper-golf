// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "BasePaperGolfVolume.h"
#include "OutOfBoundsVolume.generated.h"


/**
 * 
 */
UCLASS()
class PGGAMEPLAY_API AOutOfBoundsVolume : public ABasePaperGolfVolume
{
	GENERATED_BODY()

protected:
	virtual void OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;
};
