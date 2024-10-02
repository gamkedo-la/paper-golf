// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "PlayerStateConfigurator.h"
#include "PlayerConfig.h"
#include "Algo/RandomShuffle.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PaperGolfLogging.h"

#include "State/GolfPlayerState.h"

PG::FPlayerStateConfigurator::FPlayerStateConfigurator(UPlayerConfig* PlayerConfig)
{
	if (!ensureMsgf(PlayerConfig, TEXT("PlayerConfig is not defined")))
	{
		return;
	}

	AvailablePlayerColors = PlayerConfig->AvailablePlayerColors;
	if (ensureMsgf(!AvailablePlayerColors.IsEmpty(), TEXT("No available player colors defined in PlayerConfig: %s"), *PlayerConfig->GetName()))
	{
		Algo::RandomShuffle(AvailablePlayerColors);
	}
	else
	{
		UE_LOG(LogPaperGolfGame, Error, TEXT("No available player colors defined in PlayerConfig: %s"), *PlayerConfig->GetName());
	}
}

void PG::FPlayerStateConfigurator::AssignToPlayer(AGolfPlayerState& GolfPlayerState)
{
	AssignColorToPlayer(GolfPlayerState);
}

void PG::FPlayerStateConfigurator::AssignColorToPlayer(AGolfPlayerState& GolfPlayerState)
{
	if (AvailablePlayerColors.IsEmpty())
	{
		UE_VLOG_UELOG(&GolfPlayerState, LogPaperGolfGame, Log,
			TEXT("%s: No more available player colors to assign to player - looking to see if can recycle colors"), *GolfPlayerState.GetName());

		if (!RecycleColors())
		{
			UE_VLOG_UELOG(&GolfPlayerState, LogPaperGolfGame, Warning,
				TEXT("%s: No more available player colors to assign to player - Resetting available colors"), *GolfPlayerState.GetName());
			ResetAvailableColors();
		}

		if (AvailablePlayerColors.IsEmpty())
		{
			// already logged the empty condition in constructor
			return;
		}
	}

	check(!AvailablePlayerColors.IsEmpty());

	const FLinearColor& ColorToAssign = AvailablePlayerColors.Pop();
	AssignedColors.Add({ColorToAssign, &GolfPlayerState});

	GolfPlayerState.SetPlayerColor(ColorToAssign);
}

bool PG::FPlayerStateConfigurator::RecycleColors()
{
	bool bRecycled{};
	for(auto It = AssignedColors.CreateIterator(); It; ++It)
	{
		if (!It->PlayerState.IsValid())
		{
			AvailablePlayerColors.AddUnique(It->Color);
			It.RemoveCurrent();
			bRecycled = true;
		}
	}

	return bRecycled;
}

void PG::FPlayerStateConfigurator::ResetAvailableColors()
{
	for(const auto& Entry : AssignedColors)
	{
		AvailablePlayerColors.AddUnique(Entry.Color);
	}

	Algo::RandomShuffle(AvailablePlayerColors);
}
