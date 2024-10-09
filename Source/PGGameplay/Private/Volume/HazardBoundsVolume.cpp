// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/HazardBoundsVolume.h"

#include "Pawn/PaperGolfPawn.h"
#include "PGTags.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HazardBoundsVolume)

AHazardBoundsVolume::AHazardBoundsVolume()
{
	Type = EPaperGolfVolumeOverlapType::End;
	Tags.Add(PG::Tags::Hazard);
}

void AHazardBoundsVolume::OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnEnteredHazard.Broadcast(&PaperGolfPawn, HazardType);
}

bool AHazardBoundsVolume::CheckEndCondition_Implementation(const APaperGolfPawn* PaperGolfPawn) const
{
	check(PaperGolfPawn);
	return PaperGolfPawn->IsAtRest();
}
