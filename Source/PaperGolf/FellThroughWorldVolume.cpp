// Copyright Game Salutes. All Rights Reserved.


#include "FellThroughWorldVolume.h"

#include "PaperGolfPawn.h"
#include "GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FellThroughWorldVolume)

void AFellThroughWorldVolume::OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnClippedThroughWorld.Broadcast(&PaperGolfPawn);
}
