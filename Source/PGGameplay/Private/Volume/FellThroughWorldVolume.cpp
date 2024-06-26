// Copyright Game Salutes. All Rights Reserved.


#include "Volume/FellThroughWorldVolume.h"

#include "Pawn/PaperGolfPawn.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FellThroughWorldVolume)

void AFellThroughWorldVolume::OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnClippedThroughWorld.Broadcast(&PaperGolfPawn);
}
