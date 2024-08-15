// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/GolfMatchPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include "Net/UnrealNetwork.h"

#include <tuple>
#include <compare>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfMatchPlayerState)

int32 AGolfMatchPlayerState::GetDisplayScore() const
{
    return DisplayScore;
}

void AGolfMatchPlayerState::AwardPoints(int32 InPoints)
{
    DisplayScore += static_cast<uint8>(InPoints);
    OnDisplayScoreUpdated.Broadcast(*this);

    ForceNetUpdate();
}

bool AGolfMatchPlayerState::CompareByScore(const AGolfPlayerState& Other) const
{
    const auto CompareResult = DisplayScore <=> Other.GetDisplayScore();
    if(CompareResult == 0)
	{
        return Super::CompareByScore(Other);
	}

    // Highest match score should sort first
    return CompareResult > 0;
}

void AGolfMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGolfMatchPlayerState, DisplayScore);
}

void AGolfMatchPlayerState::OnRep_DisplayScore()
{
    UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_DisplayScore - DisplayScore=%d"), *GetName(), DisplayScore);
    OnDisplayScoreUpdated.Broadcast(*this);
}
