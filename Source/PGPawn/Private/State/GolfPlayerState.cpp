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
	DOREPLIFETIME(AGolfPlayerState, bScored);
	DOREPLIFETIME(AGolfPlayerState, PlayerColor);
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

	ForceNetUpdate();
}

void AGolfPlayerState::StartHole()
{
	Shots = 0;
	bScored = false;
	ForceNetUpdate();
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
	bScored = OtherPlayerState->bScored;
}

bool AGolfPlayerState::CompareByScore(const AGolfPlayerState& Other) const
{
	// Sort bots last
	return std::make_tuple(GetTotalShots(), IsABot(), GetPlayerName()) < 
		   std::make_tuple(Other.GetTotalShots(), Other.IsABot(), Other.GetPlayerName());
}

void AGolfPlayerState::OnRep_ScoreByHole()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ScoreByHole - ScoreByHole=%s"), *GetName(), *PG::ToString(ScoreByHole));
	OnTotalShotsUpdated.Broadcast(*this);
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void AGolfPlayerState::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory PlayerStateCategory;
	PlayerStateCategory.Category = FString::Printf(TEXT("PlayerState"));

	PlayerStateCategory.Add(TEXT("Name"), *GetPlayerName());
	PlayerStateCategory.Add(TEXT("Shots"), FString::Printf(TEXT("%d"), GetShots()));
	PlayerStateCategory.Add(TEXT("TotalShots"), FString::Printf(TEXT("%d"), GetTotalShots()));
	PlayerStateCategory.Add(TEXT("IsReadyForShot"), LoggingUtils::GetBoolString(IsReadyForShot()));

	// Decide if going to add as a top level category or child
	if (Snapshot->Status.IsEmpty())
	{
		Snapshot->Status.Add(PlayerStateCategory);
	}

	Snapshot->Status.Last().AddChild(PlayerStateCategory);
}

#endif

#pragma endregion Visual Logger
