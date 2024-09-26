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

void AGolfPlayerState::AddShot()
{
	++Shots;
	// Broadcast immediately on the server
	OnHoleShotsUpdated.Broadcast(*this);

	ForceNetUpdate();
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
	bPositionAndRotationSet = false;

	// Broadcast immediately on the server
	OnHoleShotsUpdated.Broadcast(*this);

	ForceNetUpdate();
}

void AGolfPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	DoCopyProperties(Cast<AGolfPlayerState>(PlayerState));
}

void AGolfPlayerState::CopyGameStateProperties(const AGolfPlayerState* InPlayerState)
{
	if (!IsValid(InPlayerState))
	{
		return;
	}

	SetScore(InPlayerState->Score);
	// Don't copy player name as may not want to inherit this from the other state

	DoCopyProperties(InPlayerState);

	ForceNetUpdate();
}

void AGolfPlayerState::SetActorLocationAndRotation(AActor& Actor) const
{
	if (!bPositionAndRotationSet)
	{
		return;
	}

	Actor.SetActorLocation(Position);
	Actor.SetActorRotation(Rotation);
}

void AGolfPlayerState::SetLocationAndRotation(const AActor& Actor)
{
	bPositionAndRotationSet = true;
	Position = Actor.GetActorLocation();
	Rotation = Actor.GetActorRotation();
}

void AGolfPlayerState::DoCopyProperties(const AGolfPlayerState* InPlayerState)
{
	ScoreByHole = InPlayerState->ScoreByHole;
	Shots = InPlayerState->Shots;
	bReadyForShot = InPlayerState->bReadyForShot;
	bSpectatorOnly = InPlayerState->bSpectatorOnly;
	bScored = InPlayerState->bScored;
	PlayerColor = InPlayerState->PlayerColor;

	bPositionAndRotationSet = InPlayerState->bPositionAndRotationSet;
	Position = InPlayerState->Position;
	Rotation = InPlayerState->Rotation;
}

bool AGolfPlayerState::CompareByScore(const AGolfPlayerState& Other) const
{
	// Sort bots last
	return std::make_tuple(GetTotalShots(), IsABot(), GetPlayerName()) < 
		   std::make_tuple(Other.GetTotalShots(), Other.IsABot(), Other.GetPlayerName());
}

bool AGolfPlayerState::CompareByCurrentHoleShots(const AGolfPlayerState& Other) const
{
	// Sort bots last
	return std::make_tuple(GetShots(), IsABot(), GetPlayerName()) <
		std::make_tuple(Other.GetShots(), Other.IsABot(), Other.GetPlayerName());
}

void AGolfPlayerState::OnRep_ScoreByHole()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ScoreByHole - ScoreByHole=%s"), *GetName(), *PG::ToString(ScoreByHole));
	OnTotalShotsUpdated.Broadcast(*this);
}

void AGolfPlayerState::OnRep_Shots()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_Shots - Shots=%d"), *GetName(), Shots);
	OnHoleShotsUpdated.Broadcast(*this);
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
