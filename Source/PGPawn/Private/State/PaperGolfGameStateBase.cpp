// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfGameStateBase.h"
#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "PGPawnLogging.h"

#include "State/GolfPlayerState.h"

#include "Pawn/PaperGolfPawn.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameStateBase)

void APaperGolfGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfGameStateBase, CurrentHoleNumber);
	DOREPLIFETIME(APaperGolfGameStateBase, ActivePlayer);
}

void APaperGolfGameStateBase::SetCurrentHoleNumber(int32 Hole)
{
	if (Hole == CurrentHoleNumber)
	{
		return;
	}

	CurrentHoleNumber = Hole;
	OnHoleChanged.Broadcast(CurrentHoleNumber);

	ForceNetUpdate();
}

void APaperGolfGameStateBase::SetActivePlayer(AGolfPlayerState* Player)
{
	ActivePlayer = Player;
	ForceNetUpdate();
}

void APaperGolfGameStateBase::AddPlayerState(APlayerState* PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: AddPlayerState - PlayerState=%s"), *GetName(), *LoggingUtils::GetName(PlayerState));

	Super::AddPlayerState(PlayerState);

	if (auto GolfPlayerState = Cast<AGolfPlayerState>(PlayerState); GolfPlayerState)
	{
		// Add sync listener
		if (!GolfPlayerState->OnTotalShotsUpdated.IsBoundToObject(this))
		{
			GolfPlayerState->OnTotalShotsUpdated.AddUObject(this, &APaperGolfGameStateBase::OnTotalShotsUpdated);
		}
	}
}

void APaperGolfGameStateBase::RemovePlayerState(APlayerState* PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: RemovePlayerState - PlayerState=%s"), *GetName(), *LoggingUtils::GetName(PlayerState));

	if (auto GolfPlayerState = Cast<AGolfPlayerState>(PlayerState); GolfPlayerState)
	{
		// Remove sync listener
		GolfPlayerState->OnTotalShotsUpdated.RemoveAll(this);
	}

	Super::RemovePlayerState(PlayerState);
}

TArray<AGolfPlayerState*> APaperGolfGameStateBase::GetSortedPlayerStatesByScore() const
{
	TArray<AGolfPlayerState*> GolfPlayerStates;
	GolfPlayerStates.Reserve(PlayerArray.Num());

	for (auto PlayerState : PlayerArray)
	{
		if (auto GolfPlayerState = Cast<AGolfPlayerState>(PlayerState); GolfPlayerState && !GolfPlayerState->IsSpectatorOnly())
		{
			GolfPlayerStates.Add(GolfPlayerState);
		}
	}

	GolfPlayerStates.StableSort([](const AGolfPlayerState& A, const AGolfPlayerState& B)
	{
		return A.CompareByScore(B);
	});

	return GolfPlayerStates;
}

// by default this is when everyone has scored
bool APaperGolfGameStateBase::IsHoleComplete() const
{
	for(auto PlayerState : PlayerArray)
	{
		if (auto GolfPlayerState = Cast<AGolfPlayerState>(PlayerState); GolfPlayerState)
		{
			if (!GolfPlayerState->HasScored())
			{
				return false;
			}
		}
	}

	return true;
}

void APaperGolfGameStateBase::BeginPlay()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	// subscribe to events if on server so can invoke netmulticast events so that the events are also broadcast to clients
	if (HasAuthority())
	{
		SubscribeToGolfEvents();
	}
}

void APaperGolfGameStateBase::OnHoleComplete()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnHoleComplete"), *GetName());

	DoAdditionalHoleComplete();

	MulticastOnNextHole();
}

void APaperGolfGameStateBase::OnRep_CurrentHoleNumber()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_CurrentHoleNumber - CurrentHoleNumber=%d"), *GetName(), CurrentHoleNumber);

	OnHoleChanged.Broadcast(CurrentHoleNumber);
}

void APaperGolfGameStateBase::OnTotalShotsUpdated(AGolfPlayerState& PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnTotalShotsUpdated - PlayerState=%s"), *GetName(), *PlayerState.GetName());

	UpdatedPlayerStates.AddUnique(&PlayerState);

	CheckScoreSyncState();
}

void APaperGolfGameStateBase::CheckScoreSyncState()
{
	if (AllScoresSynced())
	{
		// All player states have been updated
		// TODO: Make sure this logic works under player drops and mid-game adds - should because of the add/remove overrides
		OnScoresSynced.Broadcast(*this);

		ResetScoreSyncState();
	}
}

bool APaperGolfGameStateBase::AllScoresSynced() const
{
	return UpdatedPlayerStates.Num() == PlayerArray.Num();
}

void APaperGolfGameStateBase::ResetScoreSyncState()
{
	UpdatedPlayerStates.Reset();
}

#pragma region Client Golf Events

void APaperGolfGameStateBase::SubscribeToGolfEvents()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SubscribeToGolfEvents"), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	// Subscribe to key state changing events that are only broadcast from server otherwise
	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfCourseComplete.AddUniqueDynamic(this, &ThisClass::MulticastOnCourseComplete);
		GolfEventsSubsystem->OnPaperGolfNextHole.AddUniqueDynamic(this, &ThisClass::OnHoleComplete);
		GolfEventsSubsystem->OnPaperGolfStartHole.AddUniqueDynamic(this, &ThisClass::MulticastOnStartHole);
		GolfEventsSubsystem->OnPaperGolfPawnScored.AddUniqueDynamic(this, &ThisClass::MulticastOnPlayerScored);
	}
}

void APaperGolfGameStateBase::MulticastOnCourseComplete_Implementation()
{
	// Don't broadcast if on the server
	if (HasAuthority())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: MulticastOnCourseComplete_Implementation"), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfCourseComplete.Broadcast();
	}
}

void APaperGolfGameStateBase::MulticastOnNextHole_Implementation()
{
	// Don't broadcast if on the server
	if (HasAuthority())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: MulticastOnNextHole_Implementation"), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfNextHole.Broadcast();
	}
}

void APaperGolfGameStateBase::MulticastOnStartHole_Implementation(int32 HoleNumber)
{
	// Don't broadcast if on the server
	if (HasAuthority())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: MulticastOnStartHole_Implementation - HoleNumber=%d"), *GetName(), HoleNumber);

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfStartHole.Broadcast(HoleNumber);
	}
}

void APaperGolfGameStateBase::MulticastOnPlayerScored_Implementation(APaperGolfPawn* PlayerPawn)
{
	// Don't broadcast if on the server
	if (HasAuthority())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: MulticastOnPlayerScored_Implementation - PlayerPawn=%s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
	{
		GolfEventsSubsystem->OnPaperGolfPawnScored.Broadcast(PlayerPawn);
	}
}

#pragma endregion Client Golf Events

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void APaperGolfGameStateBase::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("GolfGameState (%s)"), *GetName());

	Category.Add(TEXT("CurrentHoleNumber"), FString::Printf(TEXT("%d"), CurrentHoleNumber));
	Category.Add(TEXT("ActivePlayer"), LoggingUtils::GetName(ActivePlayer));
	Category.Add(TEXT("UpdatedPlayerStates"), PG::ToStringObjectElements(UpdatedPlayerStates));

	Snapshot->Status.Add(Category);
}

#endif

#pragma endregion Visual Logger
