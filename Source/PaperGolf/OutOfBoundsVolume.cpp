// Copyright Game Salutes. All Rights Reserved.


#include "OutOfBoundsVolume.h"

#include "PaperGolfPawn.h"
#include "GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OutOfBoundsVolume)

void AOutOfBoundsVolume::OnPaperGolfPawnOverlap(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnOutBounds.Broadcast(&PaperGolfPawn);
}
