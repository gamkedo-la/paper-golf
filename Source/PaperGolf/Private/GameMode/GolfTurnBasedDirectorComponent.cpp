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
		*PG::ToString < TScriptInterface<IGolfController>, decltype([](const auto& Player) { return Player ? Player->ToString() : TEXT("NULL");  })> (Players));

	// TODO: Order should be based on previous hole's finish

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

	Players.Remove(GolfPlayer);
}

void UGolfTurnBasedDirectorComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: InitializeComponent"), *GetName());

	Super::InitializeComponent();

	// TODO: Disable this if just going to use BeginPlay to handle the set up or if that doesn't work then might need a function that the game mode will call to initialize
	// or we could just do it on start hole
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

	if (IsValid(PaperGolfPawn))
	{
		// Make sure that collision disabled for simulated proxies
		PaperGolfPawn->MulticastSetCollisionEnabled(false);
	}
	
	DoNextTurn();
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfPlayerScored(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfPlayerScored: PaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	check(PaperGolfPawn);

	if (auto GolfController = Cast<IGolfController>(PaperGolfPawn->GetController()); ensure(GolfController))
	{
		GolfController->MarkScored();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: OnPaperGolfShotFinished - PaperGolfPawn=%s - Controller=%s is not a AGolfPlayerController"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn), *LoggingUtils::GetName(PaperGolfPawn->GetController()));
	}

	DoNextTurn();
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
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: DoNextTurn - Hole Complete - By %s"), *GetName(),
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
			Players[i]->Spectate(NextPlayer->GetPaperGolfPawn());
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
		if (!ensureMsgf(PlayerState, TEXT("%s: DetermineClosestPlayerToHole - Player=%s; PlayerState=%s is not AGolfPlayerState"),
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
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Verbose, TEXT("%s: DetermineNextPlayer - Found closer player=(%d)%s - NewDist=%.1fm; PrevDist=%s; PrevPlayer=%s"),
				*GetName(), i, *PG::StringUtils::ToString(Player), DistanceToHole / 100,
				NextPlayer ? *FString::Printf(TEXT("%.1fm"), NextPlayer->DistanceToHole / 100) : TEXT("N/A"),
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
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: NextHole"), *GetName());

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
		if (DistanceToHole > Other.DistanceToHole)
		{
			return true;
		}

		return std::tie(Shots, Index) < std::tie(Other.Shots, Other.Index);
	}
}
