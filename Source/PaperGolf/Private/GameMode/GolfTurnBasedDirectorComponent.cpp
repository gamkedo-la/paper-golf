// Copyright Game Salutes. All Rights Reserved.


#include "GameMode/GolfTurnBasedDirectorComponent.h"

#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"

#include "Kismet/GameplayStatics.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Controller/GolfPlayerController.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"

UGolfTurnBasedDirectorComponent::UGolfTurnBasedDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UGolfTurnBasedDirectorComponent::StartHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: StartHole with %d player%s: %s"), *GetName(), Players.Num(), LoggingUtils::Pluralize(Players.Num()), *PG::ToStringObjectElements(Players));

	// TODO: Order should be based on previous hole's finish

	if (!ensureMsgf(!Players.IsEmpty(), TEXT("%s: StartHole - No players"), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: StartHole - No players"), *GetName());
		return;
	}

	ActivePlayerIndex = 0;
	ActivateNextPlayer();
}

void UGolfTurnBasedDirectorComponent::AddPlayer(AController* Player)
{
	auto PC = Cast<AGolfPlayerController>(Player);
	if(!ensureMsgf(PC, TEXT("%s: AddPlayer - Player=%s is not a AGolfPlayerController"), *GetName(), *LoggingUtils::GetName(Player)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: AddPlayer - Player=%s is not a AGolfPlayerController"), *GetName(), *LoggingUtils::GetName(Player));
		return;
	}

	Players.AddUnique(PC);
}

void UGolfTurnBasedDirectorComponent::RemovePlayer(AController* Player)
{
	auto PC = Cast<AGolfPlayerController>(Player);
	if(!ensureMsgf(PC, TEXT("%s: RemovePlayer - Player=%s is not a AGolfPlayerController"), *GetName(), *LoggingUtils::GetName(Player)))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: RemovePlayer - Player=%s is not a AGolfPlayerController"), *GetName(), *LoggingUtils::GetName(Player));
		return;
	}

	Players.Remove(PC);
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

	if(auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); GolfEventSubsystem)
	{
		GolfEventSubsystem->OnPaperGolfShotFinished.AddDynamic(this, &UGolfTurnBasedDirectorComponent::OnPaperGolfShotFinished);
		GolfEventSubsystem->OnPaperGolfPawnScored.AddDynamic(this, &UGolfTurnBasedDirectorComponent::OnPaperGolfPlayerScored);
	}
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfShotFinished(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfShotFinished: PaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	DoNextTurn();
}

void UGolfTurnBasedDirectorComponent::OnPaperGolfPlayerScored(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnPaperGolfPlayerScored: PaperGolfPawn=%s"), *GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	check(PaperGolfPawn);

	if (auto GolfPlayerController = Cast<AGolfPlayerController>(PaperGolfPawn->GetController()); ensure(GolfPlayerController))
	{
		GolfPlayerController->MarkScored();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: OnPaperGolfShotFinished - PaperGolfPawn=%s - Controller=%s is not a AGolfPlayerController"),
			*GetName(), *LoggingUtils::GetName(PaperGolfPawn), *LoggingUtils::GetName(PaperGolfPawn->GetController()));
	}

	DoNextTurn();
}

void UGolfTurnBasedDirectorComponent::DoNextTurn()
{
	ActivePlayerIndex = DetermineNextPlayer();

	if (ActivePlayerIndex == INDEX_NONE)
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: DoNextTurn - Hole Complete"), *GetName());
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

	ActivatePlayer(NextPlayer);

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

void UGolfTurnBasedDirectorComponent::ActivatePlayer(AGolfPlayerController* Player)
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ActivatePlayer - Player=%s"), *GetName(), *LoggingUtils::GetName(Player));

	if (!ensure(Player))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player is NULL"), *GetName());
		return;
	}

	// If it's the player's first shot we must spawn them
	const auto PlayerState = Player->GetPlayerState<AGolfPlayerState>();
	if(!ensureMsgf(PlayerState, TEXT("%s: ActivatePlayer - Player=%s; PlayerState=%s is not the expected AGolfPlayerState"),
		*GetName(), *LoggingUtils::GetName(Player), *LoggingUtils::GetName(Player->GetPlayerState<APlayerState>())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player=%s is not the expected AGolfPlayerState"),
			*GetName(), *LoggingUtils::GetName(Player), *LoggingUtils::GetName(Player->GetPlayerState<APlayerState>()));
		return;
	}

	if (PlayerState->GetShots() == 0)
	{
		if(!ensureMsgf(GameMode, TEXT("%s: ActivatePlayer - GameMode is NULL"), *GetName()))
		{
			UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Error, TEXT("%s: ActivatePlayer - Player=%s - GameMode is NULL"), *GetName(), *LoggingUtils::GetName(Player));
			return;
		}

		GameMode->RestartPlayer(Player);
		UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: ActivatePlayer - Player=%s - Spawning into game"), *GetName(), *LoggingUtils::GetName(Player));
	}

	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: ActivatePlayer - Player=%s - starting turn"), *GetName(), *LoggingUtils::GetName(Player));

	Player->ActivateTurn();
}

int32 UGolfTurnBasedDirectorComponent::DetermineClosestPlayerToHole() const
{
	// TODO: Determine player closest to hole
	return (ActivePlayerIndex + 1) % Players.Num();
}

int32 UGolfTurnBasedDirectorComponent::DetermineNextPlayer() const
{
	// Every player needs to have gone at least once before using closest to hole.  If a player finishes the hole, they are removed from the list

	// TODO: Use DetermineClosestPlayerToHole if all players have gone at least once

	for (int32 i = 0, InitialNextPlayerIndex = ActivePlayerIndex + 1, Len = Players.Num(); i < Len; ++i)
	{
		const auto Index = (InitialNextPlayerIndex + i) % Len;

		auto Player = Players[Index];
		// TODO: May want to make this more generic in case they hit the shot limit for instance and we implement that feature
		if (!Player->HasScored())
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

// TODO: This belongs in a separate component or should instead fire an event that is picked up by the game mode or another component on the game mode
void UGolfTurnBasedDirectorComponent::NextHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Display, TEXT("%s: NextHole"), *GetName());

	// TODO: Adjust once have multiple holes and a way to transition between them
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto Delegate = FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (auto World = GetWorld(); ensure(World))
		{
			World->ServerTravel("?Restart");
		}
	});

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, Delegate, NextHoleDelay, false);
}
