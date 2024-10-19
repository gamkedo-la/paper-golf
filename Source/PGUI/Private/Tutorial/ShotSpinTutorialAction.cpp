// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/ShotSpinTutorialAction.h"

#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

#include "Pawn/PaperGolfPawn.h"

#include "Interfaces/GolfController.h"

#include "Interfaces/FocusableActor.h"

#include "PaperGolfTypes.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotSpinTutorialAction)

namespace
{
	const TArray<FText> Messages = {
		NSLOCTEXT("ShotSpinTutorialAction", "Message1", "Press Z/X to add top or backspin to your shot")
	};
}

UShotSpinTutorialAction::UShotSpinTutorialAction()
{
	Messages = ::Messages;
	MessageDuration = 3.0f;
}

bool UShotSpinTutorialAction::IsRelevant() const
{
	// completed
	if (!Super::IsRelevant())
	{
		return false;
	}

	// If hitting full power shot will overshoot then display the shot preview tutorial
	const auto GolfController = GetGolfController();
	if (!ensure(GolfController))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: GolfController is null"), *GetName());
		return false;
	}

	const auto ShotType = GolfController->GetShotType();

	// Don't recommend spin on a close shot
	if (ShotType == EShotType::Close)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: TRUE - ShotType=%s"),
			*GetName(), *LoggingUtils::GetName(ShotType));
		return false;
	}

	const auto GolfHole = GolfController->GetCurrentGolfHole();

	if (!ensure(GolfHole))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: GolfHole is null"), *GetName());
		return false;
	}

	// If GolfHole is defined it should be focusable
	// Do not show this tutorial on the first hole
	if (IFocusableActor::Execute_GetHoleNumber(GolfHole) < 2)
	{
		UE_CVLOG_UELOG(!GolfHole->Implements<UFocusableActor>(), GetOuter(), LogPGUI, Error,
			TEXT("%s: IsRelevant: FALSE - GolfHole=%s is not a focusable actor!"),
			*GetName(), *LoggingUtils::GetName(GolfHole));

		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: FALSE - HoleNumber=%d"),
			*GetName(), IFocusableActor::Execute_GetHoleNumber(GolfHole));
		return false;
	}

	// If this is a medium shot then show the tutorial
	if (ShotType == EShotType::Medium)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: TRUE - ShotType=%s"),
			*GetName(), *LoggingUtils::GetName(ShotType));
		return true;
	}

	// For full shot see if we would overshoot - much like the ShotPreviewTutorialAction
	// TODO: Currently copied and pasted from ShotPreviewTutorialAction

	const auto PlayerPawn = GolfController->GetPaperGolfPawn();
	if (!ensure(PlayerPawn))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: Controller=%s; PlayerPawn is null"),
			*GetName(), *GolfController->ToString());
		return false;
	}

	// Make sure the hole is the current focus
	if (PlayerPawn->GetFocusActor() != GolfHole)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: FALSE - Hole is not the focus"),
			*GetName());
		return false;
	}

	// For full shot do a flick prediction to see if overshoot

	FPredictProjectilePathResult PredictPathResult;

	if (!PlayerPawn->PredictFlick(
		FFlickParams{ .ShotType = ShotType },
		{},
		PredictPathResult
	))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: FALSE - PredictPathResult did not result in a collision"),
			*GetName());
		return false;
	}

	const auto& ShotLocation = PlayerPawn->GetActorLocation();
	const auto& PredictedHitLocation = PredictPathResult.HitResult.ImpactPoint;
	const auto& HoleLocation = GolfHole->GetActorLocation();

	// Check direction of current position to hole relative to hit location

	const auto ToHole = HoleLocation - ShotLocation;
	const auto PredictedToHole = HoleLocation - PredictedHitLocation;

	const bool bWillOvershoot = (ToHole | PredictedToHole) <= 0;

	UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: %s - Based on predicted hit location: DotProduct=%f; PredictedHitLocation=%s"),
		*GetName(), LoggingUtils::GetBoolString(bWillOvershoot),
		FVector::DotProduct(ToHole.GetSafeNormal(), PredictedToHole.GetSafeNormal()), *PredictedHitLocation.ToCompactString());

	return bWillOvershoot;
}
