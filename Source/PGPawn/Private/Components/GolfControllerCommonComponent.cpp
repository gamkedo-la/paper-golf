// Copyright Game Salutes. All Rights Reserved.


#include "Components/GolfControllerCommonComponent.h"

#include "Interfaces/GolfController.h"
#include "Interfaces/FocusableActor.h"
#include "Pawn/PaperGolfPawn.h"

#include "PGPawnLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/StringUtils.h"

#include "Utils/ArrayUtils.h"

#include "PaperGolfTypes.h"


#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Kismet/GameplayStatics.h"

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

	if (auto World = GetWorld(); ensure(World))
	{
		FTimerHandle InitTimerHandle;
		World->GetTimerManager().SetTimer(InitTimerHandle, this, &ThisClass::InitFocusableActors, 0.2f);
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

EShotType UGolfControllerCommonComponent::DetermineShotType()
{
	if (!GolfHole)
	{
		return EShotType::Default;
	}

	if (!ensure(GolfController))
	{
		return EShotType::Default;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return EShotType::Default;
	}

	const auto ShotDistance = PaperGolfPawn->GetDistanceTo(GolfHole);

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

void UGolfControllerCommonComponent::SetPaperGolfPawnAimFocus()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	if (!ensure(GolfController))
	{
		return;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!GolfHole)
	{
		return;
	}

	const auto& Position = PaperGolfPawn->GetActorLocation();

	AActor* BestFocus{ GolfHole };

	if (HasLOSToFocus(Position, GolfHole))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: LOS to DefaultFocus; Setting to %s"),
			*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(GolfHole));
	}
	else
	{
		const auto ToHole = GolfHole->GetActorLocation() - Position;

		// Find closest
		float MinDist{ std::numeric_limits<float>::max() };

		for (auto FocusTarget : FocusableActors)
		{
			if (!FocusTarget)
			{
				continue;
			}
			const auto ToFocusTarget = FocusTarget->GetActorLocation() - Position;

			// Make sure we are facing the focus target
			if ((ToFocusTarget | ToHole) <= 0)
			{
				UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s: SetPaperGolfPawnAimFocus - Skipping target=%s as it is behind"),
					*GetName(), *FocusTarget->GetName());
				UE_VLOG_ARROW(GetOwner(), LogPGPawn, Verbose, Position, FocusTarget->GetActorLocation(), FColor::Orange, TEXT("Target: %s"), *FocusTarget->GetName());

				continue;
			}

			// Consider Z as don't want to aim at targets way above or below us
			const auto DistSq = ToFocusTarget.SizeSquared();
			if (DistSq < MinDist && HasLOSToFocus(Position, FocusTarget))
			{
				MinDist = DistSq;
				BestFocus = FocusTarget;
			}
		} // for

		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: BestFocus=%s"),
			*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(BestFocus));
		if (BestFocus)
		{
			UE_VLOG_ARROW(GetOwner(), LogPGPawn, Log, Position, BestFocus->GetActorLocation(), FColor::Blue, TEXT("Target: %s"), *BestFocus->GetName());
		}
	}

	PaperGolfPawn->SetFocusActor(BestFocus);
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

void UGolfControllerCommonComponent::InitFocusableActors()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GameState = World->GetGameState<APaperGolfGameStateBase>();
	if (!ensureAlwaysMsgf(GameState, TEXT("%s-%s - GetCurrentHole: GameState=%s is not APaperGolfGameStateBase"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(World->GetGameState())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s - GetCurrentHole: GameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(World->GetGameState()));
		return;
	}

	FocusableActors.Reset();
	GolfHole = nullptr;

	TArray<AActor*> InterfaceActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UFocusableActor::StaticClass(), InterfaceActors);

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: InitFocusableActors - Found %d instances of UFocusableActor in world: %s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), InterfaceActors.Num(), *PG::ToStringObjectElements(InterfaceActors));

	const auto HoleNumber = GameState->GetCurrentHoleNumber();

	for (auto Actor : InterfaceActors)
	{
		// See https://www.stevestreeting.com/2020/11/02/ue4-c-interfaces-hints-n-tips/ for why we need to avoid using Cast
		// since there are blueprint implemenations of this C++ interface
		// This already asserted with GetAllActorsWithInterface
		checkf(Actor->Implements<UFocusableActor>(),
			TEXT("%s-%s: GetAllActorsWithInterface added but Implements<UFocusableActor> returns false for %s!"), 
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Actor->GetName());

		// Filter to hole number
		if (IFocusableActor::Execute_GetHoleNumber(Actor) != HoleNumber)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose,
				TEXT("%s-%s: InitFocusableActors - Skipping %s with hole %d as it is not for hole %d"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Actor->GetName(), IFocusableActor::Execute_GetHoleNumber(Actor), HoleNumber);
			continue;
		}

		// See if this is the hole or not
		if (IFocusableActor::Execute_IsHole(Actor))
		{
			if (!GolfHole)
			{
				GolfHole = Actor;
				UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose,
					TEXT("%s-%s: InitFocusableActors - Matched GolfHole=%s for HoleNumber=%d"),
					*GetName(), *LoggingUtils::GetName(GetOwner()), *Actor->GetName(), HoleNumber);
			}
			else
			{
				UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: InitFocusableActors - Found multiple golf holes for hole number %d: %s and %s"),
					*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber, *GolfHole->GetName(), *Actor->GetName());
			}
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose,
				TEXT("%s-%s: InitFocusableActors - Matched FocusActor=%s for HoleNumber=%d"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Actor->GetName(), HoleNumber);

			FocusableActors.Add(Actor);
		}
	} // for InterfaceActors

	if (FocusableActors.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Warning,
			TEXT("%s-%s: InitFocusableActors - no non-hole instances of UFocusableActor present in world. Aim targeting impacted."),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}

	ensureMsgf(GolfHole, TEXT("%s: InitFocusableActors - No relevant AGolfHole in world for hole focus. No aim targeting will occur."),
		*GetName());
}

bool UGolfControllerCommonComponent::HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	if (!FocusActor)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(FocusActor);

	// prefer the hole if have LOS
	// TODO: Use custom trace channel or possibly alternative strategy as this doesn't always work as intended
	const bool bLOS = !World->LineTraceTestByChannel(
		Position + 200.f,
		FocusActor->GetActorLocation() + 200.f,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);

	UE_VLOG_ARROW(GetOwner(), LogPGPawn, Verbose, Position + 200.f, FocusActor->GetActorLocation() + 200.f,
		bLOS ? FColor::Green : FColor::Red, TEXT("BestFocus: %s"), *FocusActor->GetName());

	return bLOS;
}

void UGolfControllerCommonComponent::BeginTurn()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: BeginTurn"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	RegisterShotFinishedTimer();
}

void UGolfControllerCommonComponent::EndTurn()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: EndTurn"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	UnregisterShotFinishedTimer();
}
