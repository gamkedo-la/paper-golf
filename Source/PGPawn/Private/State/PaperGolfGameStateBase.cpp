// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfGameStateBase.h"
#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include "State/GolfPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameStateBase)

void APaperGolfGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaperGolfGameStateBase, CurrentHoleNumber);
	DOREPLIFETIME(APaperGolfGameStateBase, ActivePlayer);

}

void APaperGolfGameStateBase::SetActivePlayer(AGolfPlayerState* Player)
{
	ActivePlayer = Player;
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

void APaperGolfGameStateBase::OnRep_CurrentHoleNumber()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_CurrentHoleNumber - CurrentHoleNumber=%d"), *GetName(), CurrentHoleNumber);

	OnHoleChanged.Broadcast(CurrentHoleNumber);
}

void APaperGolfGameStateBase::OnTotalShotsUpdated(AGolfPlayerState& PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnTotalShotsUpdated - PlayerState=%s"), *GetName(), *PlayerState.GetName());

	UpdatedPlayerStates.AddUnique(&PlayerState);

	if(UpdatedPlayerStates.Num() == PlayerArray.Num())
	{
		// All player states have been updated
		// TODO: Make sure this logic works under player drops and mid-game adds - should because of the add/remove overrides
		OnScoresSynced.Broadcast(*this);
		UpdatedPlayerStates.Reset();
	}
}
