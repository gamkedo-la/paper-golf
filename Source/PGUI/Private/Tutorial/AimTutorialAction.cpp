// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/AimTutorialAction.h"

#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "Pawn/PaperGolfPawn.h"

#include "PaperGolfTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AimTutorialAction)

namespace
{
	const TArray<FText> Messages = {
		NSLOCTEXT("AimTutorialAction", "Message1", "Press Q/E to aim left or right"),
		NSLOCTEXT("AimTutorialAction", "Message2", "Press A/D to aim up or down")
	};
}

UAimTutorialAction::UAimTutorialAction()
{
	Messages = ::Messages;
}

bool UAimTutorialAction::IsRelevant() const
{
	// completed
	if (!Super::IsRelevant())
	{
		return false;
	}

	// If shot is occluded then display the aim tutorial
	auto PlayerPawn = GetPlayerPawn();
	if (!ensure(PlayerPawn))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: PlayerPawn is null"), *GetName());
		return false;
	}

	// Do not need to get the pitch angle as the pawn is already pointed in direction that it will shoot so pass 0
	const bool bPass = UPaperGolfPawnUtilities::TraceCurrentShotWithParameters(PlayerPawn, PlayerPawn,
		FFlickParams{
			// Do full shot to ensure simulation would catch a collision
			.ShotType = EShotType::Full
		}, 0);

	UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: TraceResult=%s -> %s"), 
		*GetName(), LoggingUtils::GetBoolString(bPass), LoggingUtils::GetBoolString(!bPass));

	return !bPass;
}
