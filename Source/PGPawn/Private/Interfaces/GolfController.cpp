// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Interfaces/GolfController.h"

#include "State/GolfPlayerState.h"
#include "Pawn/PaperGolfPawn.h"

#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PGPawnLogging.h"

#include "Components/GolfControllerCommonComponent.h"
#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfController)



void IGolfController::DoBeginPlay(const TFunction<void(UGolfEventsSubsystem&)>& ClippedThroughWorldRegistrator)
{
	UE_VLOG_UELOG(AsController(), LogPGPawn, Log, TEXT("%s: DoBeginPlay"), *ToString());

	auto Component = GetGolfControllerCommonComponent();
	check(Component);

	// Fine to use CreateRaw as the lifetime of the component is tied to this controller
	// We cannot use CreateUObject here because this class is not a UObject to the compiler even though it is inherited by controller classes
	// If we really wanted to we could CreateAWeakLambda and use AsController() as the object
	Component->Initialize(FSimpleDelegate::CreateRaw(this, &ThisClass::OnShotFinished));

	RegisterGolfSubsystemEvents(ClippedThroughWorldRegistrator);
}

AGolfPlayerState* IGolfController::GetGolfPlayerState()
{
	return AsController()->GetPlayerState<AGolfPlayerState>();
}

const AGolfPlayerState* IGolfController::GetGolfPlayerState() const
{
	return AsController()->GetPlayerState<const AGolfPlayerState>();
}

void IGolfController::RegisterGolfSubsystemEvents(const TFunction<void(UGolfEventsSubsystem&)>& ClippedThroughWorldRegistrator)
{
	auto Controller = AsController();

	auto World = Controller->GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GolfSubsystem = World->GetSubsystem<UGolfEventsSubsystem>();
	if (!ensure(GolfSubsystem))
	{
		return;
	}

	// Only handle this on server
	if (Controller->HasAuthority())
	{
		ClippedThroughWorldRegistrator(*GolfSubsystem);
	}
}

void IGolfController::OnShotFinished()
{
	UE_VLOG_UELOG(AsController(), LogPGPawn, Log, TEXT("%s: OnShotFinished"), *ToString());

	auto Controller = AsController();

	// TODO: Why in AGolfPlayerController is this IsLocalPlayerController guarding MulticastReliableSetTransform? 
	// Don't we want to do the multicast if we have authority? Changed it here

	if (auto PaperGolfPawn = GetPaperGolfPawn(); Controller->HasAuthority() && PaperGolfPawn)
	{
		UE_VLOG_UELOG(AsController(), LogPGPawn, Log,
			TEXT("%s-%s: OnShotFinished - Setting final authoritative position for pawn: %s"),
			*ToString(), *PaperGolfPawn->GetName(), *PaperGolfPawn->GetActorLocation().ToCompactString());

		PaperGolfPawn->MulticastReliableSetTransform(PaperGolfPawn->GetActorLocation(), true, PaperGolfPawn->GetActorRotation());
	}

	DoAdditionalOnShotFinished();
}

void IGolfController::DoOnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn)
{
	UE_VLOG_UELOG(AsController(), LogPGPawn, Log, TEXT("%s: OnFellThroughFloor: InPaperGolfPawn=%s"), *ToString(), *LoggingUtils::GetName(InPaperGolfPawn));

	auto PaperGolfPawn = GetPaperGolfPawn();

	if (PaperGolfPawn != InPaperGolfPawn)
	{
		return;
	}

	HandleFallThroughFloor();
}

void IGolfController::HandleFallThroughFloor()
{
	UE_VLOG_UELOG(AsController(), LogPGPawn, Log,
		TEXT("%s-%s: HandleFallThroughFloor"),
		*ToString(), *LoggingUtils::GetName(AsController()->GetPawn()));

	auto Component = GetGolfControllerCommonComponent();
	check(Component);

	if (!Component->HandleFallThroughFloor())
	{
		return;
	}

	DoAdditionalFallThroughFloor();
}

void IGolfController::AddStroke()
{
	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		UE_VLOG_UELOG(AsController(), LogPGPawn, Error, TEXT("%s: AddStroke: GolfPlayerState is null"), *ToString());
		return;
	}

	UE_VLOG_UELOG(AsController(), LogPGPawn, Log, TEXT("%s: AddStroke: %d -> %d"), *ToString(),
		GolfPlayerState->GetShots(), GolfPlayerState->GetShots() + 1);

	GolfPlayerState->AddShot();
}

bool IGolfController::IsSpectatorOnly() const
{
	auto GolfPlayerState = GetGolfPlayerState();
	if(!ensure(GolfPlayerState))
	{
		return false;
	}

	return GolfPlayerState->IsSpectatorOnly();
}

void IGolfController::SetSpectatorOnly()
{
	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	GolfPlayerState->SetSpectatorOnly();
}
