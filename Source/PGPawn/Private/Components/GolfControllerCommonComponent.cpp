// Copyright Game Salutes. All Rights Reserved.


#include "Components/GolfControllerCommonComponent.h"

#include "Interfaces/GolfController.h"
#include "Pawn/PaperGolfPawn.h"

#include "PGPawnLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/StringUtils.h"

#include "PaperGolfTypes.h"


#include "State/GolfPlayerState.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfControllerCommonComponent)

UGolfControllerCommonComponent::UGolfControllerCommonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGolfControllerCommonComponent::Initialize(const FSimpleDelegate& InOnControllerShotFinished)
{
	OnControllerShotFinished = InOnControllerShotFinished;
}

void UGolfControllerCommonComponent::BeginPlay()
{
	Super::BeginPlay();

	GolfController = GetOwner();
	if(!ensureAlwaysMsgf(GolfController, TEXT("%s-%s owner is not a IGolfController"), *GetName(), *LoggingUtils::GetName(GetOwner())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s owner is not a IGolfController"), *GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
}

void UGolfControllerCommonComponent::DestroyPawn()
{
	if (!ensure(GolfController))
	{
		return;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	auto Controller = GolfController->AsController();
	check(Controller);

	Controller->UnPossess();

	if (IsValid(PaperGolfPawn))
	{
		PaperGolfPawn->Destroy();
	}
}

EShotType UGolfControllerCommonComponent::DetermineShotType(const AActor& GolfHole)
{
	if (!ensure(GolfController))
	{
		return EShotType::Default;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return EShotType::Default;
	}

	const auto ShotDistance = PaperGolfPawn->GetDistanceTo(&GolfHole);

	const EShotType NewShotType = [&]()
	{
		if (ShotDistance > MediumShotThreshold)
		{
			return EShotType::Full;
		}
		if (ShotDistance > CloseShotThreshold)
		{
			return EShotType::Medium;
		}
		return EShotType::Close;
	}();

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: %s - DetermineShotType: %s - Distance=%fm"),
		*GetName(), *LoggingUtils::GetName(GetOwner()),
		*PaperGolfPawn->GetName(), *LoggingUtils::GetName(NewShotType), ShotDistance / 100);

	return NewShotType;
}

void UGolfControllerCommonComponent::AddToShotHistory(APaperGolfPawn* PaperGolfPawn)
{
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Warning, TEXT("%s: AddToShotHistory - PaperGolfPawn is NULL"), *GetName());
		return;
	}

	const auto& ActorLocation = PaperGolfPawn->GetActorLocation();

	const auto Size = ShotHistory.Num();

	[[maybe_unused]] const auto Index = ShotHistory.AddUnique({ ActorLocation });

	if (ShotHistory.Num() > Size)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: AddToShotHistory - PaperGolfPawn=%s - New shot added to history: %s; Count=%d"),
			*GetName(),
			*LoggingUtils::GetName(GetOwner()),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			ShotHistory.Num());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: AddToShotHistory - PaperGolfPawn=%s - Duplicate shot not added to history: %s; ExistingIndex=%d; Count=%d"),
			*GetName(),
			*LoggingUtils::GetName(GetOwner()),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			Index,
			ShotHistory.Num());
	}
}

void UGolfControllerCommonComponent::RegisterShotFinishedTimer()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(NextShotTimerHandle, this, &ThisClass::CheckForNextShot, RestCheckTickRate, true);
}

void UGolfControllerCommonComponent::UnregisterShotFinishedTimer()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(NextShotTimerHandle);
}

bool UGolfControllerCommonComponent::HandleFallThroughFloor()
{
	if (!ensure(GolfController))
	{
		return false;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning,
			TEXT("%s: HandleFallThroughFloor - PaperGolf pawn is NULL"),
			*GetName());
		return false;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log,
		TEXT("%s-%s: %s - HandleFallThroughFloor"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(PaperGolfPawn));


	const auto& Position = PaperGolfPawn->GetActorLocation() + FVector::UpVector * FallThroughFloorCorrectionTestZ;

	SetPositionTo(Position);

	return true;
}

void UGolfControllerCommonComponent::ResetShot()
{
	if (!ensure(GolfController))
	{
		return;
	}

	const auto PaperGolfPawn = GolfController->GetPaperGolfPawn();

	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->ResetRotation();
}

bool UGolfControllerCommonComponent::SetupNextShot(bool bSetCanFlick)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetupNextShot: bSetCanFlick=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LoggingUtils::GetBoolString(bSetCanFlick));

	if (!ensure(GolfController))
	{
		return false;
	}

	if (!ensureMsgf(GolfController->IsReadyForNextShot(), TEXT("%s-%s: %s - SetupNextShot - Not ready for next shot"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(GolfController->AsController()->GetPawn())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: %s - SetupNextShot - Not ready for next shot"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(GolfController->AsController()->GetPawn()));
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose,
			TEXT("%s-%s: CheckAndSetupNextShot - Skip - No PaperGolf pawn"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return false;
	}

	if (bSetCanFlick)
	{
		PaperGolfPawn->SetCollisionEnabled(true);
	}

	PaperGolfPawn->SetUpForNextShot();

	ResetShot();
	PaperGolfPawn->SnapToGround();

	AddToShotHistory(PaperGolfPawn);

	if (bSetCanFlick)
	{
		if (auto GolfPlayerState = GolfController->GetGolfPlayerState(); GetOwner()->HasAuthority() && ensure(GolfPlayerState))
		{
			GolfPlayerState->SetReadyForShot(true);
		}

		// Broadcast new upright ready position and rotation
		if (GetOwner()->HasAuthority())
		{
			PaperGolfPawn->MulticastReliableSetTransform(PaperGolfPawn->GetActorLocation(), true, PaperGolfPawn->GetActorRotation());
		}
	}

	return true;
}

void UGolfControllerCommonComponent::SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation)
{
	if (!ensure(GolfController))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log,
		TEXT("%s-%s: SetPositionTo - Position=%s; Rotation=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Position.ToCompactString(), *PG::StringUtils::ToString(OptionalRotation));

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	PaperGolfPawn->SetTransform(Position, OptionalRotation);
	PaperGolfPawn->MulticastReliableSetTransform(Position, OptionalRotation.IsSet(), OptionalRotation ? OptionalRotation.GetValue() : FRotator::ZeroRotator);
}

void UGolfControllerCommonComponent::CheckForNextShot()
{
	if (!ensure(GolfController))
	{
		return;
	}

	if (GolfController->HasScored())
	{
		UnregisterShotFinishedTimer();
		return;
	}

	if (!GolfController->IsReadyForNextShot())
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	UnregisterShotFinishedTimer();

	OnControllerShotFinished.ExecuteIfBound();

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		// This will call a function on server from game mode to set up next turn - we use above RPC to make sure 
		GolfEventSubsystem->OnPaperGolfShotFinished.Broadcast(PaperGolfPawn);
	}
}

void UGolfControllerCommonComponent::BeginTurn()
{
	RegisterShotFinishedTimer();
}

void UGolfControllerCommonComponent::EndTurn()
{
	UnregisterShotFinishedTimer();
}
