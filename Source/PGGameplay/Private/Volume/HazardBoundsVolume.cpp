// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Volume/HazardBoundsVolume.h"

#include "Pawn/PaperGolfPawn.h"
#include "PGTags.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include "Utils/PGAudioUtilities.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGGameplayLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HazardBoundsVolume)

AHazardBoundsVolume::AHazardBoundsVolume()
{
	Type = EPaperGolfVolumeOverlapType::End;
	Tags.Add(PG::Tags::Hazard);

	// So we can send RPCs
	bAlwaysRelevant = true;
	bReplicates = true;
	NetDormancy = ENetDormancy::DORM_DormantAll;
}

void AHazardBoundsVolume::OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents)
{
	GolfEvents.OnPaperGolfPawnEnteredHazard.Broadcast(&PaperGolfPawn, HazardType);

	if (HazardSound)
	{
		MulticastConditionTriggered(PaperGolfPawn.GetActorLocation());
	}
}

bool AHazardBoundsVolume::CheckEndCondition_Implementation(const APaperGolfPawn* PaperGolfPawn) const
{
	check(PaperGolfPawn);
	return PaperGolfPawn->IsAtRest();
}

void AHazardBoundsVolume::MulticastConditionTriggered_Implementation(const FVector_NetQuantize& Location)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: MulticastConditionTriggered: Location=%s"), *GetName(), *Location.ToCompactString());

	if (GetNetMode() != NM_DedicatedServer)
	{
		UPGAudioUtilities::PlaySfxAtLocation(this, Location, HazardSound);
	}
}
