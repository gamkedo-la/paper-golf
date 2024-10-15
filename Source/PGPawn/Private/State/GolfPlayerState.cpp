// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/GolfPlayerState.h"

#include "Net/UnrealNetwork.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"
#include "Utils/ArrayUtils.h"

#include "Pawn/PaperGolfPawn.h"

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

void AGolfPlayerState::UpdateShotCount(int32 DeltaCount)
{
	check(DeltaCount != 0);

	Shots += DeltaCount;

	// Broadcast immediately on the server
	OnHoleShotsUpdated.Broadcast(*this, Shots - DeltaCount);

	ForceNetUpdate();
}

void AGolfPlayerState::SetReadyForShot(bool bReady)
{
	if (bReadyForShot == bReady)
	{
		return;
	}

	bReadyForShot = bReady;
	OnReadyForShotUpdated.Broadcast(*this);

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
	const auto PreviousShots = Shots;

	Shots = 0;
	bScored = false;
	bPositionAndRotationSet = false;

	// Broadcast immediately on the server
	OnHoleShotsUpdated.Broadcast(*this, PreviousShots);

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

	SetScore(InPlayerState->GetScore());
	// Don't copy player name as may not want to inherit this from the other state

	DoCopyProperties(InPlayerState);

	ForceNetUpdate();
}

void AGolfPlayerState::SetPawnLocationAndRotation(APaperGolfPawn& PlayerPawn) const
{
	if (!bPositionAndRotationSet)
	{
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SetPawnLocationAndRotation - Position=%s, Rotation=%s"), *GetName(), *Position.ToString(), *Rotation.ToString());

	PlayerPawn.SetTransform(Position, Rotation);
}

void AGolfPlayerState::SetLocationAndRotationFromPawn(const APaperGolfPawn& PlayerPawn)
{
	bPositionAndRotationSet = true;
	Position = PlayerPawn.GetPaperGolfPosition();
	Rotation = PlayerPawn.GetPaperGolfRotation();

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: SetLocationAndRotationFromPawn - Position=%s, Rotation=%s"), *GetName(), *Position.ToString(), *Rotation.ToString());
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

bool AGolfPlayerState::CompareByCurrentHoleShots(const AGolfPlayerState& Other, bool bIncludeTurnActivation) const
{
	// Sort bots last
	return std::make_tuple(bIncludeTurnActivation ? GetShotsIncludingCurrent() : GetShots(), IsABot(), GetPlayerName()) <
		std::make_tuple(bIncludeTurnActivation ? Other.GetShotsIncludingCurrent() : GetShots(), Other.IsABot(), Other.GetPlayerName());
}

void AGolfPlayerState::OnRep_ScoreByHole()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ScoreByHole - ScoreByHole=%s"), *GetName(), *PG::ToString(ScoreByHole));
	OnTotalShotsUpdated.Broadcast(*this);
}

void AGolfPlayerState::OnRep_Shots(uint8 PreviousShots)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_Shots - Shots=%d; PreviousShots=%d"), *GetName(), Shots, PreviousShots);
	OnHoleShotsUpdated.Broadcast(*this, PreviousShots);
}

void AGolfPlayerState::OnRep_ReadyForShot()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_ReadyForShot - bReadyForShot=%s"), *GetName(), LoggingUtils::GetBoolString(bReadyForShot));
	OnReadyForShotUpdated.Broadcast(*this);
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void AGolfPlayerState::DoGrabDebugSnapshot(FVisualLogEntry* Snapshot, FVisualLogStatusCategory* ParentCategory) const
{
	FVisualLogStatusCategory PlayerStateCategory;
	PlayerStateCategory.Category = FString::Printf(TEXT("PlayerState"));

	PlayerStateCategory.Add(TEXT("Name"), *GetPlayerName());
	PlayerStateCategory.Add(TEXT("Shots"), FString::Printf(TEXT("%d"), GetShots()));
	PlayerStateCategory.Add(TEXT("TotalShots"), FString::Printf(TEXT("%d"), GetTotalShots()));
	PlayerStateCategory.Add(TEXT("IsReadyForShot"), LoggingUtils::GetBoolString(IsReadyForShot()));
	PlayerStateCategory.Add(TEXT("IsSpectatorOnly"), LoggingUtils::GetBoolString(IsSpectatorOnly()));
	PlayerStateCategory.Add(TEXT("IsScored"), LoggingUtils::GetBoolString(HasScored()));
	PlayerStateCategory.Add(TEXT("PlayerColor"), PlayerColor.ToString());

	if (HasAuthority())
	{
		PlayerStateCategory.Add(TEXT("PositionAndRotationSet"), LoggingUtils::GetBoolString(bPositionAndRotationSet));
		if (bPositionAndRotationSet)
		{
			PlayerStateCategory.Add(TEXT("Position"), Position.ToString());
			PlayerStateCategory.Add(TEXT("Rotation"), Rotation.ToString());
		}
	}

	if (ParentCategory)
	{
		ParentCategory->AddChild(PlayerStateCategory);
	}
	else
	{
		Snapshot->Status.Add(PlayerStateCategory);
	}
}

#endif

#pragma endregion Visual Logger
