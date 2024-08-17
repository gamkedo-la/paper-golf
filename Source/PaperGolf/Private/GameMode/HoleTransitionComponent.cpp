// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/HoleTransitionComponent.h"

// TODO: May not need the game mode reference here if using events
#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "Utils/StringUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"

#include "Kismet/GameplayStatics.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Interfaces/GolfController.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Golf/GolfHole.h"

#include "PlayerStart/GolfPlayerStart.h"
#include "GameFramework/PlayerStart.h"

#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(HoleTransitionComponent)

UHoleTransitionComponent::UHoleTransitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UHoleTransitionComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

void UHoleTransitionComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: InitializeComponent"), *GetName());

	Super::InitializeComponent();

	Init();
}

void UHoleTransitionComponent::Init()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: Init"), *GetName());

	GameMode = Cast<AGameModeBase>(GetOwner());
	if (!ensureMsgf(GameMode, TEXT("%s: Owner=%s is not AGameModeBase"), *GetName(), *LoggingUtils::GetName(GetOwner())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: Owner=%s is not a game mode"), *GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	GameState = Cast<APaperGolfGameStateBase>(GameMode->GameState);
	if (!ensureMsgf(GameState, TEXT("%s: GameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameMode->GameState)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: GameState=%s is not APaperGolfGameStateBase"), *GetName(), *LoggingUtils::GetName(GameMode->GameState));
		return;
	}

	check(GetOwner()->HasAuthority());

	LastHoleIndex = GameState->GetCurrentHoleNumber() - 1;

	if(ensureMsgf(LastHoleIndex >= 0, TEXT("%s: Init - LastHoleIndex=%d is invalid, defaulting to 0"), *GetName(), LastHoleIndex))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: Init - LastHoleIndex=%d"), *GetName(), LastHoleIndex);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: Init - LastHoleIndex=%d is invalid, defaulting to 0"), *GetName(), LastHoleIndex);
		LastHoleIndex = 0;
	}

	InitCachedData();
	RegisterEventHandlers();
}

void UHoleTransitionComponent::InitCachedData()
{
	InitPlayerStarts();
	InitHoles();
}

void UHoleTransitionComponent::InitPlayerStarts()
{
	RelevantPlayerStarts.Reset();

	for (TObjectIterator<AGolfPlayerStart> Itr; Itr; ++Itr)
	{
		// Take first game world - only one instance should exist
		UWorld* InstanceWorld = Itr->GetWorld();

		//World Check to avoid getting objects from the editor world when doing PIE
		if (Itr->GetWorld() != GetWorld())
		{
			continue;
		}

		RelevantPlayerStarts.Add(*Itr);
	}

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: InitPlayerStarts - Found %d relevant player start%s: %s"),
		*GetName(),
		RelevantPlayerStarts.Num(),
		LoggingUtils::Pluralize(RelevantPlayerStarts.Num()),
		*PG::ToStringObjectElements(RelevantPlayerStarts)
	);
}

void UHoleTransitionComponent::InitHoles()
{
	GolfHoles = AGolfHole::GetAllWorldHoles(this, true);

	// Make sure all holes are unique and sequential

#if !UE_BUILD_SHIPPING

	AGolfHole* PreviousHole{};
	int32 PreviousHoleNumber{};

	for(auto It = GolfHoles.CreateIterator(); It; ++It)
	{
		auto Hole = *It;
		if (!ensureMsgf(Hole, TEXT("%s: InitHoles - Found null hole in GolfHoles"), *GetName()))
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: InitHoles - Found null hole in GolfHoles"), *GetName());

			It.RemoveCurrent();
			continue;
		}

		const auto HoleNumber = AGolfHole::Execute_GetHoleNumber(Hole);
		if (!ensureMsgf(HoleNumber > 0, TEXT("%s: InitHoles - Hole %s has invalid hole number %d"), *GetName(), *LoggingUtils::GetName(Hole), HoleNumber))
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: InitHoles - Hole %s has invalid hole number %d"), *GetName(), *LoggingUtils::GetName(Hole), HoleNumber);

			It.RemoveCurrent();
			continue;
		}

		if (PreviousHole)
		{
			if (!ensureMsgf(HoleNumber == PreviousHoleNumber + 1, TEXT("%s: InitHoles - Hole %s has hole number %d but previous hole %s has hole number %d"),
				*GetName(), *LoggingUtils::GetName(Hole), HoleNumber, *LoggingUtils::GetName(PreviousHole), PreviousHoleNumber))
			{
				UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: InitHoles - Hole %s has hole number %d but previous hole %s has hole number %d"),
					*GetName(), *LoggingUtils::GetName(Hole), HoleNumber, *LoggingUtils::GetName(PreviousHole), PreviousHoleNumber);
				// remove duplicate holes but allow gaps
				if (HoleNumber == PreviousHoleNumber)
				{
					It.RemoveCurrent();
					continue;
				}
			}
		}

		PreviousHole = Hole;
		PreviousHoleNumber = HoleNumber;
	} // for
#endif
}

void UHoleTransitionComponent::RegisterEventHandlers()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		GolfEventSubsystem->OnPaperGolfNextHole.AddDynamic(this, &ThisClass::OnNextHole);
	}
}

void UHoleTransitionComponent::ResetGameStateForNextHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ResetGameStateForNextHole"), *GetName());

	if (!ensure(GameMode))
	{
		return;
	}

	// This will call reset on all the appropriate actors
	GameMode->ResetLevel();
}

AActor* UHoleTransitionComponent::ChoosePlayerStart(AController* Player)
{
	if (!Player)
	{
		return nullptr;
	}

#if WITH_EDITOR
	if (auto PlayerStart = FindPlayFromHereStart(Player); PlayerStart)
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ChoosePlayerStart - Using PIE PlayerFromHere PlayerStart=%s"),
			*GetName(), *LoggingUtils::GetName(PlayerStart));
		return PlayerStart;
	}
#endif

	if (!ensureAlwaysMsgf(GameState, TEXT("%s: ChoosePlayerStart - GameState not initialized - returning nullptr!"), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ChoosePlayerStart - GameState not initialized - returning nullptr!"), *GetName());
		return nullptr;
	}

	const auto HoleNumber = GameState->GetCurrentHoleNumber();

	auto MatchedPlayerStart = RelevantPlayerStarts.FindByPredicate([HoleNumber](const AGolfPlayerStart* PlayerStart)
	{
		return PlayerStart->GetHoleNumber() == HoleNumber;
	});

	if (!MatchedPlayerStart)
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Warning, TEXT("%s: ChoosePlayerStart - No matching player start found for hole %d out of %d candidate%s - returning nullptr!"),
			*GetName(), HoleNumber, RelevantPlayerStarts.Num(), LoggingUtils::Pluralize(RelevantPlayerStarts.Num()));
		return nullptr;
	}

#if WITH_EDITOR
	// ensure single match
	const auto Matches = RelevantPlayerStarts.FilterByPredicate([HoleNumber](const AGolfPlayerStart* PlayerStart)
	{
		return PlayerStart->GetHoleNumber() == HoleNumber;
	});

	if (Matches.Num() != 1)
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Warning, TEXT("%s: ChoosePlayerStart - Multiple matching player starts found for hole %d: %s - returning first match!"),
			*GetName(), HoleNumber, *PG::ToStringObjectElements(Matches));
	}

#endif

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ChoosePlayerStart - Matched player start=%s hole %d"),
		*GetName(), *LoggingUtils::GetName(*MatchedPlayerStart), HoleNumber);

	return *MatchedPlayerStart;
}

void UHoleTransitionComponent::OnNextHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnNextHole"), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::OnNextHoleTimer, NextHoleDelay);
}

void UHoleTransitionComponent::OnNextHoleTimer()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnNextHoleTimer"), *GetName());

	// Determine next hole and if we are at the end, broadcast the course complete event
	if (!ensure(IsValid(GameState)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: OnNextHoleTimer - GameState not valid"), *GetName());
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); 
	
	if (!ensure(GolfEventSubsystem))
	{
		return;
	}

	++LastHoleIndex;

	if (LastHoleIndex >= GolfHoles.Num())
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: OnNextHoleTimer - Course complete"), *GetName());

		GolfEventSubsystem->OnPaperGolfCourseComplete.Broadcast();

		return;
	}

	const auto NextHoleNumber = AGolfHole::Execute_GetHoleNumber(GolfHoles[LastHoleIndex]);
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: OnNextHoleTimer - Transitioning to Hole Number=%d"), *GetName(), NextHoleNumber);

	GameState->SetCurrentHoleNumber(NextHoleNumber);
	ResetGameStateForNextHole();

	GolfEventSubsystem->OnPaperGolfStartHole.Broadcast(NextHoleNumber);
}

#if WITH_EDITOR
// See LyraPlayerStartComponent.cpp in the Lyra project
APlayerStart* UHoleTransitionComponent::FindPlayFromHereStart(AController* Player)
{
	// Only 'Play From Here' for a player controller, bots etc. should all spawn from normal spawn points.
	if (Player->IsA<APlayerController>())
	{
		if (auto World = GetWorld(); ensure(World))
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (auto PlayerStart = *It; PlayerStart)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
						return PlayerStart;
					}
				}
			}
		}
	}

	return nullptr;
}
#endif
