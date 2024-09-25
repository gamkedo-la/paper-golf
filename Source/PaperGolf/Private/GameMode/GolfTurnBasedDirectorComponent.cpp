// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/GolfTurnBasedDirectorComponent.h"

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

#include <utility>

#include <limits>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfTurnBasedDirectorComponent)

namespace
{
	struct FGolfPlayerOrderState
	{
		int32 Index{};
		int32 Shots{};
		float DistanceToHole{};

		bool operator < (const FGolfPlayerOrderState& Other) const;
	};
}
UGolfTurnBasedDirectorComponent::UGolfTurnBasedDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UGolfTurnBasedDirectorComponent::StartHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: StartHole with %d player%s: %s"), *GetName(), Players.Num(), LoggingUtils::Pluralize(Players.Num()), 
		*PG::ToString<TScriptInterface<IGolfController>, decltype([](const auto& Player) -> FString { return Player ? Player->ToString() : TEXT("NULL");  }), decltype(Players)::AllocatorType>(Players));

	if (!ensureMsgf(!Players.IsEmpty(), TEXT("%s: StartHole - No players"), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: StartHole - No players"), *GetName());
		return;
	}

	CurrentHole = AGolfHole::GetCurrentHole(this);
	if(!ensureMsgf(CurrentHole, TEXT("%s: StartHole - CurrentHole is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: StartHole - CurrentHole is NULL"), *GetName());
		return;
	}

	InitializePlayersForHole();

	// Don't start at index 0 as human players may be set to spectate or first player may be set to spectate
	ActivePlayerIndex = DetermineNextPlayer();
	ActivateNextPlayer();
}

void UGolfTurnBasedDirectorComponent::AddPlayer(AController* Player)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: AddPlayer - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	const TScriptInterface<IGolfController> GolfPlayer{ Player };

	if(!ensureMsgf(GolfPlayer, TEXT("%s: AddPlayer - Player=%s is not a IGolfController"), *GetName(), *LoggingUtils::GetName(Player)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: AddPlayer - Player=%s is not a IGolfController"), *GetName(), *LoggingUtils::GetName(Player));
		return;
	}

	if(bSkipHumanPlayers && Player->IsPlayerController())
	{
		GolfPlayer->SetSpectatorOnly();

		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: AddPlayer - Player=%s set to spectator only as game mode set to skip human players"), *GetName(), *LoggingUtils::GetName(Player));
	}

	// TODO: Need to mark player if they join in middle of hole since need to then join as a spectator until the next hole
	// This will be handled in 114-account-for-late-joining
	Players.AddUnique(GolfPlayer);
}

void UGolfTurnBasedDirectorComponent::RemovePlayer(AController* Player)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: RemovePlayer - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	const TScriptInterface<IGolfController> GolfPlayer{ Player };
	if(!ensureMsgf(GolfPlayer, TEXT("%s: RemovePlayer - Player=%s is not a IGolfController"), *GetName(), *LoggingUtils::GetName(Player)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: RemovePlayer - Player=%s is not a IGolfController"), *GetName(), *LoggingUtils::GetName(Player));
		return;
	}

	// invalid active player index if GolfPlayer is the active player
	TScriptInterface<IGolfController> CurrentActivePlayer{};

	if (ActivePlayerIndex != INDEX_NONE)
	{
		checkf(ActivePlayerIndex >= 0 && ActivePlayerIndex < Players.Num(), 
			TEXT("%s: ActivePlayerIndex=%d >= Players.Num()=%d"), *GetName(), ActivePlayerIndex, Players.Num());
		CurrentActivePlayer = Players[ActivePlayerIndex];
	}
	
	Players.Remove(GolfPlayer);

	if (Players.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: RemovePlayer - No more players - exiting game"), *GetName());
		// TODO: Exit game - maybe not necessary in listen server
		return;
	}

	if (CurrentActivePlayer == GolfPlayer.GetInterface())
	{
		// if the active player dropped then must select the next player that would have gone
		DoNextTurn();
	}
	// If there is an active player, then we need to update the index after removal of the other player
	else if (CurrentActivePlayer)
	{
		ActivePlayerIndex = Players.IndexOfByKey(CurrentActivePlayer);
	}
	else
	{
		// If current active player is not defined then ActivePlayerIndex should always be INDEX_NONE
		// as these should always be in sync
		check(ActivePlayerIndex == INDEX_NONE);
	}
}

void UGolfTurnBasedDirectorComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: InitializeComponent"), *GetName());

	Super::InitializeComponent();

	// TODO: Disable this if just going to use BeginPlay to handle the set up or if that doesn't work then might need a function that the game mode will call to initialize
	// or we could just do it on start hole
}

int32 UGolfTurnBasedDirectorComponent::GetNumberOfActivePlayers() const
{
	int32 NumActivePlayers{};
	for (const auto& Player : Players)
	{
		if (Player && !Player->IsSpectatorOnly())
		{
			++NumActivePlayers;
		}
	}

	return NumActivePlayers;
}

void UGolfTurnBasedDirectorComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	GameMode = Cast<APaperGolfGameModeBase>(GetOwner());
	if (!ensureMsgf(GameMode, TEXT("%s: Owner=%s is not APaperGolfGameModeBase game mode"), *GetName(), *LoggingUtils::GetName(GetOwner())))
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

	bPlayersNeedInitialSort = true;

	RegisterEventHandlers();
}

void UGolfTurnBasedDirectorComponent::RegisterEventHandlers()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if(auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		GolfEventSubsystem->OnPaperGolfShotFinished.AddDynamic(this, &ThisClass::OnPaperGolfShotFinished);
		GolfEventSubsystem->OnPaperGolfPawnScored.AddDynamic(this, &ThisClass::OnPaperGolfPlayerScored);
		GolfEventSubsystem->OnPaperGolfPawnEnteredHazard.AddDynamic(this, &ThisClass::OnPaperGolfEnteredHazard);
	}
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfShotFinished(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfShotFinished: PaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	bool bIsPlayerTurn{};

	if (IsValid(PaperGolfPawn))
	{
		if (auto GolfController = Cast<IGolfController>(PaperGolfPawn->GetController()); GolfController)
		{
			// Cache this as the turn might be over before the score event comes in
			PlayerPawnToController.Add(PaperGolfPawn, PaperGolfPawn->GetController());
			bIsPlayerTurn = IsActivePlayer(GolfController);
		}
	}
	
	// Only do next turn if the current pawn is the active player as there is a possible race condition between finishing shots and detecting if player scored
	if(bIsPlayerTurn)
	{
		DoNextTurn();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfShotFinished - Skipping do next turn as PaperGolfPawn=%s is not the active player"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn));
	}
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfPlayerScored(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfPlayerScored: PaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	check(PaperGolfPawn);

	// Try to find the controller from the pawn, but if we got unpossessed then look for the cached controller from when the shot finished (turn started too early in the case of first turn as pawn may not have spawned)
	IGolfController* GolfController = Cast<IGolfController>(PaperGolfPawn->GetController());
	if (!GolfController)
	{
		auto ControllerFindResult = PlayerPawnToController.Find(PaperGolfPawn);
		if (ControllerFindResult)
		{
			GolfController = Cast<IGolfController>(ControllerFindResult->Get());
		}
	}

	bool bIsPlayerTurn{};

	if (ensure(GolfController))
	{
		bIsPlayerTurn = IsActivePlayer(GolfController);

		GolfController->MarkScored();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: OnPaperGolfShotFinished - PaperGolfPawn=%s - Controller=%s is not a AGolfPlayerController"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn), *LoggingUtils::GetName(PaperGolfPawn->GetController()));
	}

	// Only do next turn if it was currently our turn as may have selected next turn before scoring detected
	if (bIsPlayerTurn)
	{
		DoNextTurn();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfPlayerScored - Skipping do next turn as PaperGolfPawn=%s is not the active player"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn));
	}
}

bool UGolfTurnBasedDirectorComponent::IsActivePlayer(const IGolfController* Player) const
{
	if (!Player)
	{
		return false;
	}

	if (!ensure(GameState))
	{
		return false;
	}

	return GameState->GetActivePlayer() == Player->GetGolfPlayerState();
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfEnteredHazard(APaperGolfPawn* PaperGolfPawn, EHazardType HazardType)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfEnteredHazard: PaperGolfPawn=%s; HazardType=%s"),
		*GetName(), *LoggingUtils::GetName(PaperGolfPawn), *LoggingUtils::GetName(HazardType));
	if (auto GolfController = Cast<IGolfController>(PaperGolfPawn->GetController()); ensure(GolfController))
	{
		if (GolfController->HandleOutOfBounds())
		{
			// One stroke penalty - TODO: May want to pull this up to another component or main game mode
			GolfController->AddStroke();
		}
	}
}

void UGolfTurnBasedDirectorComponent::DoNextTurn()
{
	ActivePlayerIndex = DetermineNextPlayer();

	check(GameState);

	// If there are no more players to go or if the game rules determine the hole is complete - e.g. match play the final rankings already determined, then finish the hole
	if (ActivePlayerIndex == INDEX_NONE || GameState->IsHoleComplete())
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: DoNextTurn - Hole Complete - %s"), *GetName(),
			ActivePlayerIndex == INDEX_NONE ? TEXT("No more unfinished players") : TEXT("Early finish via game rules"));
		NextHole();

		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: DoNextTurn - Next player index=%d"), *GetName(), ActivePlayerIndex);
	ActivateNextPlayer();
}

void UGolfTurnBasedDirectorComponent::ActivateNextPlayer()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ActivateNextPlayer - ActivePlayerIndex=%d"), *GetName(), ActivePlayerIndex);

	checkf(ActivePlayerIndex >= 0 && ActivePlayerIndex < Players.Num(), TEXT("%s: ActivateNextPlayer - ActivePlayerIndex=%d is out of range"), *GetName(), ActivePlayerIndex);
	auto NextPlayer = Players[ActivePlayerIndex];

	ActivatePlayer(NextPlayer.GetInterface());

	// other players will spectate this player
	for(int32 i = 0; i < Players.Num(); ++i)
	{
		if (i != ActivePlayerIndex)
		{
			// TODO: It's possible that the pawn won't be set yet
			// This is actually happening because the pawn is not set yet
			Players[i]->Spectate(NextPlayer->GetPaperGolfPawn(), NextPlayer->GetGolfPlayerState());
		}
	}
}

void UGolfTurnBasedDirectorComponent::ActivatePlayer(IGolfController* Player)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ActivatePlayer - Player=%s"), *GetName(), *PG::StringUtils::ToString(Player));

	if (!ensure(Player))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player is NULL"), *GetName());
		return;
	}

	// If it's the player's first shot we must spawn them
	const auto PlayerState = Player->GetGolfPlayerState();
	if(!ensureMsgf(PlayerState, TEXT("%s: ActivatePlayer - Player=%s; PlayerState=%s is not the expected AGolfPlayerState"),
		*GetName(), *PG::StringUtils::ToString(Player), *LoggingUtils::GetName(Cast<AController>(Player) ? Cast<AController>(Player)->GetPlayerState<APlayerState>() : nullptr)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player=%s is not the expected AGolfPlayerState"),
			*GetName(), *PG::StringUtils::ToString(Player), *LoggingUtils::GetName(Cast<AController>(Player) ? Cast<AController>(Player)->GetPlayerState<APlayerState>() : nullptr));
		return;
	}

	check(GameState);
	GameState->SetActivePlayer(PlayerState);

	if (PlayerState->GetShots() == 0)
	{
		if(!ensureMsgf(GameMode, TEXT("%s: ActivatePlayer - GameMode is NULL"), *GetName()))
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player=%s - GameMode is NULL"), *GetName(), *PG::StringUtils::ToString(Player));
			return;
		}

		GameMode->RestartPlayer(Cast<AController>(Player));
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: ActivatePlayer - Player=%s - Spawning into game"), *GetName(), *PG::StringUtils::ToString(Player));

		Player->StartHole();
	}

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ActivatePlayer - Player=%s - starting turn"), *GetName(), *PG::StringUtils::ToString(Player));

	Player->ActivateTurn();
}

int32 UGolfTurnBasedDirectorComponent::DetermineNextPlayer() const
{
	checkf(CurrentHole, TEXT("%s: DetermineNextPlayer - CurrentHole is NULL"), *GetName());

	TOptional<FGolfPlayerOrderState> NextPlayer;

	struct FIncrementer
	{
		FIncrementer(int32& i) :i(i) {}
		~FIncrementer() { ++i;  }

		int32& i;
	};

	for (int32 i = 0; const auto& Player : Players)
	{
		// Ensure that i is incremented in all code paths
		FIncrementer _(i);

		if (Player->HasScored() || Player->IsSpectatorOnly())
		{
			// Exclude finished players or those that are only spectating
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Verbose, TEXT("%s: DetermineNextPlayer - Skip Player=(%d)%s is finished=%s or spectating=%s"),
				*GetName(), i, *PG::StringUtils::ToString(Player), LoggingUtils::GetBoolString(Player->HasScored()), LoggingUtils::GetBoolString(Player->IsSpectatorOnly()));
			continue;
		}

		const auto PlayerState = Player->GetGolfPlayerState();
		if (!ensureMsgf(PlayerState, TEXT("%s: DetermineNextPlayer - Player=%s; PlayerState=%s is not AGolfPlayerState"),
			*GetName(), *PG::StringUtils::ToString(Player),
			*LoggingUtils::GetName(Cast<AController>(Player.GetObject()) ? Cast<AController>(Player.GetObject())->GetPlayerState<APlayerState>() : nullptr)))
		{
			continue;
		}

		if (PlayerState->GetShots() == 0)
		{
			// Everyone needs to go once first
			NextPlayer =
			{
				.Index = i,
				.Shots = 0,
				.DistanceToHole = std::numeric_limits<float>::max()
			};

			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Verbose, TEXT("%s: DetermineNextPlayer - NextPlayer=(%d)%s - First shot"), *GetName(), i, *PG::StringUtils::ToString(Player));
			break;
		}

		const auto Pawn = Player->GetPaperGolfPawn();
		if (!Pawn)
		{
			continue;
		}

		const auto DistanceToHole = CurrentHole->GetSquaredHorizontalDistanceTo(Pawn);

		FGolfPlayerOrderState CurrentPlayerState
		{
			.Index = i,
			.Shots = PlayerState->GetShots(),
			.DistanceToHole = DistanceToHole
		};

		if(!NextPlayer || CurrentPlayerState < *NextPlayer)
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Verbose, TEXT("%s: DetermineNextPlayer - Found better match player=(%d)%s - NewDist=%.1fm; PrevDist=%s; PrevPlayer=%s"),
				*GetName(), i, *PG::StringUtils::ToString(Player), FMath::Sqrt(DistanceToHole) / 100,
				NextPlayer ? *FString::Printf(TEXT("%.1fm"), FMath::Sqrt(NextPlayer->DistanceToHole) / 100) : TEXT("N/A"),
				NextPlayer ? *FString::Printf(TEXT("(%d)%s"), NextPlayer->Index, *PG::StringUtils::ToString(Players[NextPlayer->Index])) : TEXT("N/A")
			);

			NextPlayer = CurrentPlayerState;
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: DetermineNextPlayer - NextPlayer=%d"), *GetName(), NextPlayer ? NextPlayer->Index : INDEX_NONE);

	return NextPlayer ? NextPlayer->Index : INDEX_NONE;
}

void UGolfTurnBasedDirectorComponent::NextHole()
{
	++HolesCompleted;

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: NextHole - HolesCompleted=%d"), *GetName(), HolesCompleted);

	check(GameState);
	GameState->SetActivePlayer(nullptr);

	MarkPlayersFinishedHole();

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		GolfEventSubsystem->OnPaperGolfNextHole.Broadcast();
	}
}

void UGolfTurnBasedDirectorComponent::InitializePlayersForHole()
{
	PlayerPawnToController.Reset();

	SortPlayersForNextHole();

	for (auto Player : Players)
	{
		if (auto PlayerState = Player->GetGolfPlayerState(); ensureMsgf(PlayerState,
			TEXT("%s: InitializePlayersForHole - Player=%s does not have GolfPlayerState"), *GetName(), *PG::StringUtils::ToString(Player)))
		{
			PlayerState->StartHole();
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: InitializePlayersForHole - Player=%s does not have GolfPlayerState"),
				*GetName(), *PG::StringUtils::ToString(Player));
		}
	}
}

void UGolfTurnBasedDirectorComponent::SortPlayersForNextHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: SortPlayersForNextHole"), *GetName());

	if (bPlayersNeedInitialSort)
	{
		Players.Sort([](const TScriptInterface<IGolfController>& First, const TScriptInterface<IGolfController>& Second)
		{
			const auto FirstPlayerState = First->GetGolfPlayerState();
			const auto SecondPlayerState = Second->GetGolfPlayerState();

			if (!FirstPlayerState)
			{
				return false;
			}
			if (!SecondPlayerState)
			{
				return true;
			}

			// Do an initial score comparison so that secondary criteria are used to match the HUD
			return FirstPlayerState->CompareByScore(*SecondPlayerState);
		});

		bPlayersNeedInitialSort = false;
	}

	// Players ordered based on lowest score from previous hole, maintaining previous order if there was a tie
	Players.StableSort([this, CompletedHoles = this->HolesCompleted](const TScriptInterface<IGolfController>& First, const TScriptInterface<IGolfController>& Second)
	{
		const auto FirstPlayerState = First->GetGolfPlayerState();
		const auto SecondPlayerState = Second->GetGolfPlayerState();

		const auto StateIsValid = [CompletedHoles](AGolfPlayerState* State)
		{
			return State && State->GetNumCompletedHoles() == CompletedHoles;
		};

		if (!StateIsValid(FirstPlayerState))
		{
			return false;
		}
		if (!StateIsValid(SecondPlayerState))
		{
			return true;
		}

		return FirstPlayerState->GetLastCompletedHoleScore() < SecondPlayerState->GetLastCompletedHoleScore();
	});
}

void UGolfTurnBasedDirectorComponent::MarkPlayersFinishedHole()
{
	for (auto Player : Players)
	{
		if (auto PlayerState = Player->GetGolfPlayerState(); ensureMsgf(PlayerState,
			TEXT("%s: MarkPlayersFinishedHole - Player=%s does not have GolfPlayerState"), *GetName(), *PG::StringUtils::ToString(Player)))
		{
			PlayerState->FinishHole();
		}
		else
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: MarkPlayersFinishedHole - Player=%s does not have GolfPlayerState"),
				*GetName(), *PG::StringUtils::ToString(Player));
		}
	}
}

namespace
{
	bool FGolfPlayerOrderState::operator<(const FGolfPlayerOrderState& Other) const
	{
		// Make error tolerance 1m so that paper footballs close together will prioritize those with fewer shots
		if (FMath::IsNearlyEqual(DistanceToHole, Other.DistanceToHole, 100.0f))
		{
			return std::tie(Shots, Index) < std::tie(Other.Shots, Other.Index);
		}

		return DistanceToHole > Other.DistanceToHole;
	}
}
