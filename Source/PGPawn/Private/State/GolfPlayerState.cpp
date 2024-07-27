// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/GolfPlayerState.h"

#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "Utils/ArrayUtils.h"

#include <tuple>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfPlayerState)

AGolfPlayerState::AGolfPlayerState()
{
	NetUpdateFrequency = 10.0f;
}

void AGolfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfPlayerState, Shots);
	DOREPLIFETIME(AGolfPlayerState, bReadyForShot);
	DOREPLIFETIME(AGolfPlayerState, ScoreByHole);
	DOREPLIFETIME(AGolfPlayerState, bSpectatorOnly);
}

int32 AGolfPlayerState::GetTotalShots() const
{
	int32 Total{};
	for(auto HoleScore : ScoreByHole)
	{
		Total += HoleScore;
	}

	return Total;
}

void AGolfPlayerState::FinishHole()
{
	ScoreByHole.Add(Shots);

	// Broadcast immediately on the server
	OnTotalShotsUpdated.Broadcast(*this);
}

void AGolfPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	auto OtherPlayerState = Cast<AGolfPlayerState>(PlayerState);
	if (!IsValid(OtherPlayerState))
	{
		return;
	}

	ScoreByHole = OtherPlayerState->ScoreByHole;
	Shots = OtherPlayerState->Shots;
	bReadyForShot = OtherPlayerState->bReadyForShot;
	bSpectatorOnly = OtherPlayerState->bSpectatorOnly;
}

bool AGolfPlayerState::CompareByScore(const AGolfPlayerState& Other) const
{
	return std::make_tuple(GetTotalShots(), GetPlayerName()) < 
		   std::make_tuple(Other.GetTotalShots(), Other.GetPlayerName());
}

void AGolfPlayerState::OnRep_ScoreByHole()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ScoreByHole - ScoreByHole=%s"), *GetName(), *PG::ToString(ScoreByHole));
	OnTotalShotsUpdated.Broadcast(*this);
}
