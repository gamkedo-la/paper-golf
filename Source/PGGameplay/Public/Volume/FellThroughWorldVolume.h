// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BasePaperGolfVolume.h"
#include "FellThroughWorldVolume.generated.h"

/**
 * 
 */
UCLASS()
class PGGAMEPLAY_API AFellThroughWorldVolume : public ABasePaperGolfVolume
{
	GENERATED_BODY()
	
protected:
	virtual void OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) override;
};
