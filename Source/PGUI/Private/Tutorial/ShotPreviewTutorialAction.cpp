// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/ShotPreviewTutorialAction.h"

#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

#include "Pawn/PaperGolfPawn.h"

#include "Interfaces/GolfController.h"

#include "PaperGolfTypes.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotPreviewTutorialAction)

namespace
{
	const TArray<FText> Messages = {
		NSLOCTEXT("ShotPreviewTutorialAction", "Message1", "Press RMB to toggle shot preview"),
		NSLOCTEXT("ShotPreviewTutorialAction", "Message2", "Use mouse wheel to change preview power"),
		NSLOCTEXT("ShotPreviewTutorialAction", "Message3", "Green line in power meter shows target shot power")
	};
}

UShotPreviewTutorialAction::UShotPreviewTutorialAction()
{
	Messages = ::Messages;
}

bool UShotPreviewTutorialAction::IsRelevant() const
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

	// If recommended shot is not a full shot then show the tutorial as probably want to not hit full power
	const auto ShotType = GolfController->GetShotType();

	if (ShotType != EShotType::Full)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: IsRelevant: TRUE - ShotType=%s"),
			*GetName(), *LoggingUtils::GetName(ShotType));
		return true;
	}

	const auto GolfHole = GolfController->GetCurrentGolfHole();
	if (!ensure(GolfHole))
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: GolfHole is null"), *GetName());
		return false;
	}

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
