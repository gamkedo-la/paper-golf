// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/GolfMatchPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include "Net/UnrealNetwork.h"

#include <tuple>
#include <compare>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfMatchPlayerState)

namespace
{
	// store 18 holes - need 5 bits - use the 5 highest bits
	// left shift this to highest bits
	constexpr uint32 ChangeBits = 5; // 18 holes possibly in future
	constexpr uint32 UpdateMaskShift = 32 - ChangeBits;

	constexpr uint32 UpdateMask = 0b11111 << UpdateMaskShift;
	constexpr uint32 ScoreMask = ~UpdateMask;
}

int32 AGolfMatchPlayerState::GetDisplayScore() const
{
    return DisplayScore & ScoreMask;
}

int32 AGolfMatchPlayerState::GetNumScoreChanges() const
{
	return (DisplayScore & UpdateMask) >> UpdateMaskShift;
}

void AGolfMatchPlayerState::SetDisplayScore(int32 Points, int32 NumChanges)
{
	DisplayScore = static_cast<uint32>(Points | (NumChanges << UpdateMaskShift));
}

void AGolfMatchPlayerState::AwardPoints(int32 InPoints)
{
	// Need to encode a change bit in order to force replication
	const int32 DisplayPts = GetDisplayScore() + InPoints;
	const int32 NumChanges = GetNumScoreChanges() + 1;

	SetDisplayScore(DisplayPts, NumChanges);

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: AwardPoints - InPoints=%d - TotalPoints=%d; NumChanges=%d"), *GetName(), InPoints, DisplayPts, NumChanges);

    OnDisplayScoreUpdated.Broadcast(*this);

    ForceNetUpdate();
}

bool AGolfMatchPlayerState::CompareByScore(const AGolfPlayerState& Other) const
{
    const auto CompareResult = GetDisplayScore() <=> Other.GetDisplayScore();
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

void AGolfMatchPlayerState::DoCopyProperties(const AGolfPlayerState* InPlayerState)
{
	Super::DoCopyProperties(InPlayerState);

	auto InMatchPlayerState = Cast<AGolfMatchPlayerState>(InPlayerState);
	if (!IsValid(InMatchPlayerState))
	{
		return;
	}

	DisplayScore = InMatchPlayerState->DisplayScore;
}

void AGolfMatchPlayerState::OnRep_DisplayScore()
{
    UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnRep_DisplayScore - Points=%d; NumScoreChanges=%d"), *GetName(), GetDisplayScore(), GetNumScoreChanges());
    OnDisplayScoreUpdated.Broadcast(*this);
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG

void AGolfMatchPlayerState::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	check(!Snapshot->Status.IsEmpty());

	auto& PlayerStateCategory = Snapshot->Status.Last();

	PlayerStateCategory.Add(TEXT("DisplayScore"), FString::Printf(TEXT("%d"), GetDisplayScore()));
	PlayerStateCategory.Add(TEXT("NumScoreChanges"), FString::Printf(TEXT("%d"), GetNumScoreChanges()));
}

#endif

#pragma endregion Visual Logger
