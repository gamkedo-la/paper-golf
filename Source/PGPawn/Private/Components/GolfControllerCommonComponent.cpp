// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/GolfControllerCommonComponent.h"

#include "Interfaces/GolfController.h"
#include "Interfaces/FocusableActor.h"
#include "Pawn/PaperGolfPawn.h"

#include "PGPawnLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/StringUtils.h"

#include "Utils/ArrayUtils.h"

#include "Utils/CollisionUtils.h"
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
		auto GameState = World->GetGameState<APaperGolfGameStateBase>();
		if (!ensureAlwaysMsgf(GameState, TEXT("%s-%s - BeginPlay: GameState=%s is not APaperGolfGameStateBase"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(World->GetGameState())))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s - BeginPlay: GameState=%s is not APaperGolfGameStateBase"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(World->GetGameState()));

			return;
		}

		// If on server, game state is updated immediately during game mode start so can just invoke OnHoleChanged directly
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakGameState = MakeWeakObjectPtr(GameState)]()
		{
			if (auto GameState = WeakGameState.Get(); GameState)
			{
				OnHoleChanged(GameState->GetCurrentHoleNumber());
			}
		}));

		// If not server, then have to listen for when game state changes holes so that we can init the focus actors correctly
		if (!GetOwner()->HasAuthority())
		{
			GameState->OnHoleChanged.AddUObject(this, &ThisClass::OnHoleChanged);
		}
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

	// Unregister any active timers
	UnregisterShotFinishedTimer();
}

AActor* UGolfControllerCommonComponent::GetShotFocusActor(EShotFocusType ShotFocusType) const
{
	if(ShotFocusType == EShotFocusType::Hole)
	{
		return GolfHole;
	}

	if (!ensure(GolfController))
	{
		return GolfHole;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return GolfHole;
	}

	auto FocusActor = PaperGolfPawn->GetFocusActor();

	if (ensure(FocusActor))
	{
		return FocusActor;
	}

	return GolfHole;
}

EShotType UGolfControllerCommonComponent::DetermineShotType(EShotFocusType ShotFocusType)
{
	const auto FocusActor = GetShotFocusActor(ShotFocusType);

	if (!FocusActor)
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

	const auto ShotDistance = PaperGolfPawn->GetDistanceTo(FocusActor);

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

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: %s - DetermineShotType(%s): %s - Distance=%fm to FocusActor=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()),
		*PaperGolfPawn->GetName(), *LoggingUtils::GetName(ShotFocusType), *LoggingUtils::GetName(NewShotType), ShotDistance / 100, *LoggingUtils::GetName(FocusActor));

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

	if (auto BestFocus = GetBestFocusActor(); BestFocus)
	{
		PaperGolfPawn->SetFocusActor(BestFocus);
	}
}

AActor* UGolfControllerCommonComponent::GetBestFocusActor(TArray<FShotFocusScores>* OutFocusScores) const
{
	if (!ensure(GolfController))
	{
		return nullptr;
	}

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return nullptr;
	}

	auto World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (!GolfHole)
	{
		return nullptr;
	}

	const auto& Position = PaperGolfPawn->GetActorLocation();

	AActor* BestFocus{ GolfHole };

	if (IFocusableActor::Execute_IsPreferredFocus(GolfHole) && HasLOSToFocus(Position, GolfHole))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: LOS to DefaultFocus; Setting to %s"),
			*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(GolfHole));

		if (OutFocusScores)
		{
			OutFocusScores->Add({ GolfHole, 0.0f });
		}
		else
		{
			return GolfHole;
		}
	}

	float MinDist{ std::numeric_limits<float>::max() };

	for (auto FocusTarget : FocusableActors)
	{
		if (!FocusTarget)
		{
			continue;
		}

		const auto ToFocusTarget = FocusTarget->GetActorLocation() - Position;
		const auto ToFocusTargetDist = FMath::Max(ToFocusTarget.Size(), 0.01f);
		const auto ToFocusTargetDir = ToFocusTarget / ToFocusTargetDist;
		const auto& FocusForwardDirection = FocusTarget->GetActorForwardVector();

		const auto FocusAlignment = FocusForwardDirection | ToFocusTargetDir;
		const auto FocusMinAlignment = IFocusableActor::Execute_GetMinCosAngle(FocusTarget);

		// Make sure we are facing the focus target
		if (FocusAlignment < FocusMinAlignment)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s: SetPaperGolfPawnAimFocus - Skipping target=%s as DotProduct=%f < FocusMinAlignment=%f"),
				*GetName(), *FocusTarget->GetName(), FocusAlignment, FocusMinAlignment);
			UE_VLOG_ARROW(GetOwner(), LogPGPawn, Verbose, Position, FocusTarget->GetActorLocation(), FColor::Orange, TEXT("Target (ALIGNMENT=%.1fd): %s"),
				FMath::RadiansToDegrees(FMath::Acos(FocusAlignment)), *FocusTarget->GetName());

			continue;
		}

		// Don't consider Z when checking for min distance
		const auto DistSq2D = ToFocusTarget.SizeSquared2D();
		const auto MinDistSq2D = FMath::Square(IFocusableActor::Execute_GetMinDistance2D(FocusTarget));

		if (DistSq2D < MinDistSq2D)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s: SetPaperGolfPawnAimFocus - Skipping target=%s as too close to it - Dist2D=%fm < MinDist=%fm"),
				*GetName(), *FocusTarget->GetName(), FMath::Sqrt(DistSq2D) / 100, FMath::Sqrt(MinDistSq2D) / 100);
			UE_VLOG_ARROW(GetOwner(), LogPGPawn, Verbose, Position, FocusTarget->GetActorLocation(), FColor::Yellow, TEXT("Target (TOO CLOSE: %.1fm, %.1fd): %s"),
				FMath::Sqrt(DistSq2D) / 100, FMath::RadiansToDegrees(FMath::Acos(FocusAlignment)), *FocusTarget->GetName());

			continue;
		}

		UE_VLOG_LOCATION(GetOwner(), LogPGPawn, Verbose, FocusTarget->GetActorLocation() + 100 * FVector::ZAxisVector, 5.0f, FColor::Green, TEXT("Target (%.1fd, %1fm)"),
			FMath::RadiansToDegrees(FMath::Acos(FocusAlignment)), FMath::Sqrt(DistSq2D) / 100, *FocusTarget->GetName());

		// Consider Z as don't want to aim at targets way above or below us
		if (OutFocusScores && HasLOSToFocus(Position, FocusTarget))
		{
			OutFocusScores->Add({ FocusTarget, static_cast<float>(ToFocusTargetDist) });
			if (ToFocusTargetDist < MinDist)
			{
				MinDist = ToFocusTargetDist;
				BestFocus = FocusTarget;
			}
		}
		else if (!OutFocusScores && ToFocusTargetDist < MinDist && HasLOSToFocus(Position, FocusTarget))
		{
			MinDist = ToFocusTargetDist;
			BestFocus = FocusTarget;
		}
	} // for

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: SetPaperGolfPawnAimFocus: BestFocus=%s"),
		*GetName(), *PaperGolfPawn->GetName(), *LoggingUtils::GetName(BestFocus));
	if (BestFocus)
	{
		UE_VLOG_ARROW(GetOwner(), LogPGPawn, Log, Position, BestFocus->GetActorLocation(), FColor::Blue, TEXT("Best Focus: %s"), *BestFocus->GetName());
	}

	if (OutFocusScores)
	{
		OutFocusScores->Sort([](const auto& First, const auto& Second)
		{
			return First.Score < Second.Score;
		});
	}

	return BestFocus;
}

void UGolfControllerCommonComponent::RegisterShotFinishedTimer()
{
	if (NextShotTimerHandle.IsValid())
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

	OnFlickHandle = PaperGolfPawn->OnFlick.AddWeakLambda(this, [this, World, Pawn = MakeWeakObjectPtr(PaperGolfPawn)]()
	{
		LastFlickTime = World->GetTimeSeconds();
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: OnFlick - Time=%fs"), *GetName(), *LoggingUtils::GetName(Pawn), LastFlickTime);
	});
	WeakPaperGolfPawn = PaperGolfPawn;

	FirstRestCheckPassTime = -1.0f;

	World->GetTimerManager().SetTimer(NextShotTimerHandle, this, &ThisClass::CheckForNextShot, RestCheckTickRate, true);
}

void UGolfControllerCommonComponent::UnregisterShotFinishedTimer()
{
	if (!NextShotTimerHandle.IsValid())
	{
		return;
	}

	if (auto PaperGolfPawn = WeakPaperGolfPawn.Get(); PaperGolfPawn && OnFlickHandle.IsValid())
	{
		PaperGolfPawn->OnFlick.Remove(OnFlickHandle);
	}

	FirstRestCheckPassTime = -1.0f;
	OnFlickHandle.Reset();
	WeakPaperGolfPawn.Reset();

	auto World = GetWorld();
	if (!World)
	{
		NextShotTimerHandle.Invalidate();
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


	const auto& Position = PaperGolfPawn->GetPaperGolfPosition() + FVector::UpVector * FallThroughFloorCorrectionTestZ;

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
}

void UGolfControllerCommonComponent::CheckForNextShot()
{
	// Make sure we didn't get unregistered in this frame
	if (!NextShotTimerHandle.IsValid())
	{
		return;
	}

	if (!ensure(GolfController))
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	// Check that enough time has elapsed
	if (World->GetTimeSeconds() - LastFlickTime < MinFlickElapsedTimeForShotFinished)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose,
			TEXT("%s-%s: CheckForNextShot - Skip - Flick in progress: time remaining=%fs"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), MinFlickElapsedTimeForShotFinished - (World->GetTimeSeconds() - LastFlickTime));
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

	auto PaperGolfPawn = GolfController->GetPaperGolfPawn();
	if (!PaperGolfPawn)
	{
		return;
	}

	const auto CurrentTimeSeconds = World->GetTimeSeconds();

	if (FirstRestCheckPassTime < 0)
	{
		FirstRestCheckPassTime = CurrentTimeSeconds;
	}

	if (const auto DeltaTime = CurrentTimeSeconds - FirstRestCheckPassTime; DeltaTime < RestCheckTriggerDelay)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose,
			TEXT("%s-%s: CheckForNextShot - Skip - Waiting for rest check delay: time remaining=%fs"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), RestCheckTriggerDelay - DeltaTime);

		return;
	}

	UnregisterShotFinishedTimer();

	OnControllerShotFinished.ExecuteIfBound();

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
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

	const auto HoleNumber = GameState->GetCurrentHoleNumber();

	if (HoleNumber == LastHoleNumber)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: InitFocusableActors - HoleNumber=%d is unchanged. Skipping."),
			*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber);
		return;
	}

	FocusableActors.Reset();
	GolfHole = nullptr;

	TArray<AActor*> InterfaceActors;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UFocusableActor::StaticClass(), InterfaceActors);

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: InitFocusableActors - HoleNumber=%d - Found %d candidate instances of UFocusableActor in world: %s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber, InterfaceActors.Num(), *PG::ToStringObjectElements(InterfaceActors));


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
				UE_VLOG_LOCATION(GetOwner(), LogPGPawn, Verbose, Actor->GetActorLocation(), 10.f, FColor::Turquoise, TEXT("Hole: %d"), HoleNumber);
			}
			else
			{
				UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: InitFocusableActors - Found multiple golf holes for hole number %d: %s and %s"),
					*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber, *GolfHole->GetName(), *Actor->GetName());
			}

			// Add the hole as a regular focusable actor
			if (!IFocusableActor::Execute_IsPreferredFocus(Actor))
			{
				FocusableActors.Add(Actor);
			}
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose,
				TEXT("%s-%s: InitFocusableActors - Matched FocusActor=%s for HoleNumber=%d"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Actor->GetName(), HoleNumber);

			UE_VLOG_LOCATION(GetOwner(), LogPGPawn, Verbose, Actor->GetActorLocation(), 10.f, FColor::Turquoise, TEXT("Focus: %d"), HoleNumber);

			FocusableActors.Add(Actor);
		}
	} // for InterfaceActors

	if (!FocusableActors.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log,
			TEXT("%s-%s: InitFocusableActors - HoleNumber=%d; GolfHole=%s; Found %d total focusable actors: %s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber, *LoggingUtils::GetName(GolfHole), 
			FocusableActors.Num(), *PG::ToStringObjectElements(FocusableActors));
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Warning,
			TEXT("%s-%s: InitFocusableActors - no non-hole instances of UFocusableActor present in world. Aim targeting impacted."),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}

	ensureMsgf(GolfHole, TEXT("%s: InitFocusableActors - No relevant AGolfHole in world for hole focus. No aim targeting will occur."),
		*GetName());

	LastHoleNumber = HoleNumber;
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

	const auto& FocusActorLocation = FocusActor->GetActorLocation();

	// Try direct LOS first before doing offsets
	const auto LOSTest = [&](const auto& Start, const auto& End)
	{
		return !World->LineTraceTestByChannel(
			Start,
			End,
			PG::CollisionChannel::FlickTraceType,
			QueryParams
		);
	};

	bool bLOS = LOSTest(Position, FocusActorLocation);

	if (!bLOS)
	{
		const auto TraceStartLocation = Position + FVector::ZAxisVector * FocusTraceStartOffset;
		const auto TraceEndLocation = FocusActor->GetActorLocation() + FVector::ZAxisVector * FocusTraceEndOffset;

		bLOS = LOSTest(TraceStartLocation, TraceEndLocation);
	}

	UE_VLOG_ARROW(GetOwner(), LogPGPawn, Verbose, Position + 200.f * FVector::ZAxisVector, FocusActor->GetActorLocation() + 200.f * FVector::ZAxisVector,
		bLOS ? FColor::Green : FColor::Red, TEXT("LOS: %s"), *FocusActor->GetName());

#if !NO_LOGGING
#if !ENABLE_VISUAL_LOG
	constexpr bVisualLogger = false;
#else
	const bool bVisualLogger = FVisualLogger::IsRecording();
#endif

	if (bVisualLogger || UE_LOG_ACTIVE(LogPGPawn,Verbose))
	{
		if (bLOS)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s-%s: HasLOSToFocus(%s,%s) = TRUE"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Position.ToCompactString(), *LoggingUtils::GetName(FocusActor));
		}
		else
		{
			FHitResult HitResult;
			World->LineTraceSingleByChannel(
				HitResult,
				Position + FVector::ZAxisVector * FocusTraceStartOffset,
				FocusActor->GetActorLocation() + FVector::ZAxisVector * FocusTraceEndOffset,
				PG::CollisionChannel::FlickTraceType,
				QueryParams
			);

			UE_VLOG_UELOG(GetOwner(), LogPGPawn, Verbose, TEXT("%s-%s: HasLOSToFocus(%s,%s) = FALSE : Hit %s-%s at %s"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Position.ToCompactString(), *LoggingUtils::GetName(FocusActor),
				*LoggingUtils::GetName(HitResult.GetActor()), *LoggingUtils::GetName(HitResult.GetComponent()),
				*HitResult.ImpactPoint.ToCompactString());
			UE_VLOG_LOCATION(GetOwner(), LogPGPawn, Verbose, HitResult.ImpactPoint, 10.f, FColor::Red, TEXT("LOS: %s - Hit"), *LoggingUtils::GetName(FocusActor));
		}
	}
#endif

	return bLOS;
}

void UGolfControllerCommonComponent::OnHoleChanged(int32 HoleNumber)
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: OnHoleChanged - HoleNumber=%d"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), HoleNumber);

	InitFocusableActors();

	// Make sure we immediately reset the aim focus if we are the active player after the hole changed
	// If we are not the active player, then it will happen when it is our turn
	check(GolfController);
	if (GolfController->IsActivePlayer())
	{
		SetPaperGolfPawnAimFocus();
		GolfController->ResetShot();
	}
}

void UGolfControllerCommonComponent::BeginTurn()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: BeginTurn"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	// Always reset the state when activating turn - this fixes and physics offset issues
	if(ensure(GolfController))
	{
		if (auto PaperGolfPawn = GolfController->GetPaperGolfPawn(); PaperGolfPawn /* && !PaperGolfPawn->IsAtRest()*/)
		{
			//UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s-%s: BeginTurn - Resetting shot state as paper golf pawn is not at rest"),
			//	*GetName(), *LoggingUtils::GetName(GetOwner()));

			// Always reset the state when activating turn - this fixes and physics offset issues
			PaperGolfPawn->SetUpForNextShot();

			if (GetOwner()->HasAuthority())
			{
				PaperGolfPawn->OnTurnStarted();
			}
		}
	}

	RegisterShotFinishedTimer();
}

void UGolfControllerCommonComponent::EndTurn()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: EndTurn"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	UnregisterShotFinishedTimer();
}

void UGolfControllerCommonComponent::Reset()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: Reset"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	ShotHistory.Reset();
	LastFlickTime = 0.f;

	InitFocusableActors();
}

void UGolfControllerCommonComponent::OnScored()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: OnScored"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	if (auto GolfPlayerState = GolfController->GetGolfPlayerState(); GetOwner()->HasAuthority() && ensure(GolfPlayerState))
	{
		GolfPlayerState->SetHasScored(true);
	}

	UnregisterShotFinishedTimer();
}
