// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "State/PaperGolfMatchGameState.h"

#include "State/GolfMatchPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"

#include "Utils/ArrayUtils.h"
#include "PGPawnLogging.h"

#include "PGConstants.h"

#include <limits>
#include <array>
#include <algorithm>

const FGolfMatchScoring APaperGolfMatchGameState::DefaultScoring{ 0, { 1 } };


void APaperGolfMatchGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (auto GolfPlayerState = Cast<AGolfMatchPlayerState>(PlayerState); GolfPlayerState)
	{
		// Add sync listener
		if (!GolfPlayerState->OnDisplayScoreUpdated.IsBoundToObject(this))
		{
			GolfPlayerState->OnDisplayScoreUpdated.AddUObject(this, &ThisClass::OnDisplayScoreUpdated);
		}
	}
}

void APaperGolfMatchGameState::RemovePlayerState(APlayerState* PlayerState)
{
	if (auto GolfPlayerState = Cast<AGolfMatchPlayerState>(PlayerState); GolfPlayerState)
	{
		// Remove sync listener
		GolfPlayerState->OnDisplayScoreUpdated.RemoveAll(this);
	}

	Super::RemovePlayerState(PlayerState);
}

bool APaperGolfMatchGameState::AllScoresSynced() const
{
	if (UpdatedMatchPlayerStates.Num() == PlayerArray.Num())
	{
		return Super::AllScoresSynced();
	}

	return false;
}

void APaperGolfMatchGameState::ResetScoreSyncState()
{
	Super::ResetScoreSyncState();

	UpdatedMatchPlayerStates.Reset();
}

APaperGolfMatchGameState::FGolfMatchPlayerStateArray APaperGolfMatchGameState::GetGolfMatchStatePlayerArray() const
{
	FGolfMatchPlayerStateArray MatchPlayerStates;
	MatchPlayerStates.Reserve(PlayerArray.Num());

	for (auto PlayerState : PlayerArray)
	{
		if (auto GolfPlayerState = Cast<AGolfMatchPlayerState>(PlayerState); GolfPlayerState)
		{
			MatchPlayerStates.Add(GolfPlayerState);
		}
	}

	return MatchPlayerStates;
}

const FGolfMatchScoring& APaperGolfMatchGameState::GetScoreConfig(int32 NumPlayers) const
{
	// select the appropriate scoring criteria based on the number of players
	const FGolfMatchScoring* ScoringCriteria = ScoringConfig.FindByPredicate([NumPlayers](const FGolfMatchScoring& Criteria)
	{
		return Criteria.NumPlayers == NumPlayers;
	});

	if (!ensureMsgf(ScoringCriteria, TEXT("No scoring criteria found for %d players"), NumPlayers))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Error, TEXT("%s: DoAdditionalHoleComplete - No scoring criteria found for %d players - using default \"winner take all\""), *GetName(), NumPlayers);
		// Default to a config that awards one point for winner
		ScoringCriteria = &DefaultScoring;
	}

	check(ScoringCriteria);

	return *ScoringCriteria;
}

bool APaperGolfMatchGameState::IsHoleComplete() const
{
	// Match scores are determined once all remaining players have scored
	auto MatchPlayerStates = GetGolfMatchStatePlayerArray();

	if (MatchPlayerStates.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogPGPawn, Warning, TEXT("%s: IsHoleComplete - TRUE - No match state players!"), *GetName());
		return true;
	}

	int32 AllScoredCount{};
	std::array<int32, PG::MaxPlayers> PlayerScores;

	for(auto PlayerState : MatchPlayerStates)
	{
		if(PlayerState->HasScored())
		{
			checkf(AllScoredCount < PlayerScores.max_size(), TEXT("%s: AllScoredCount >= PlayerScores.max_size()=%d"), *GetName(), PlayerScores.max_size());
			PlayerScores[AllScoredCount] = PlayerState->GetShots();

			++AllScoredCount;
		}
	}

	// If all players have scored then the match scores can for sure be determined

	if(AllScoredCount == MatchPlayerStates.Num())
	{
		UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: IsHoleComplete - TRUE - All %d player%s have scored"),
			*GetName(), AllScoredCount, LoggingUtils::Pluralize(AllScoredCount));
		return true;
	}

	if (bAlwaysAllowFinish)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: IsHoleComplete - FALSE - bAlwaysAllowFinish=TRUE and %d out of %d player%s scored"),
			*GetName(), AllScoredCount, MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num()));

		return false;
	}

	// Determine if all the awards can already be determined, this means that the remaining unfinished players
	// have taken as many shots as the finished player with the lowest award

	const auto NumAwards = FMath::Min(GetScoreConfig(MatchPlayerStates.Num()).Points.Num(), MatchPlayerStates.Num());

	if (!ensureMsgf(NumAwards > 0, TEXT("%s: IsHoleComplete - ScoringCriteria has no entries for %d player%s"),
		*GetName(), MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num())))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Error,
			TEXT("%s: DoAdditionalHoleComplete - No scoring criteria found for %d players"),
			*GetName(), MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num()));
		return false;
	}

	if (AllScoredCount < NumAwards)
	{
		UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: IsHoleComplete - FALSE - All %d awards have not been determined as only %d player%s scored"),
			*GetName(), NumAwards, AllScoredCount, LoggingUtils::Pluralize(AllScoredCount));

		return false;
	}

	// Find nth lowest score which is the highest awared score
	std::nth_element(PlayerScores.begin(), PlayerScores.begin() + NumAwards, PlayerScores.begin() + AllScoredCount);

	const auto ThresholdScore = PlayerScores[NumAwards - 1];

	for (auto PlayerState : MatchPlayerStates)
	{
		if (!PlayerState->HasScored() && PlayerState->GetShots() < ThresholdScore)
		{
			UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: IsHoleComplete - FALSE - Player %s has not scored and has less than the threshold score %d at award=%d for %d player%s"),
					*GetName(), *PlayerState->GetName(), ThresholdScore, NumAwards, MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num()));
			return false;
		}
	}

	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: IsHoleComplete - TRUE - All %d player%s have scored or have taken as many shots as the lowest award=%d; ThresholdScore=%d; Shots=%s"),
		*GetName(), MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num()), NumAwards, ThresholdScore,
		*PG::ToString(MatchPlayerStates, [](const AGolfMatchPlayerState* PlayerState) { return FString::Printf(TEXT("%d"), PlayerState->GetShots()); }));

	// Be sure to add an extra shot so that if doing a tie-breaker they lose to the winning players
	for (auto PlayerState : MatchPlayerStates)
	{
		if (!PlayerState->HasScored())
		{
			PlayerState->AddShot();
			// Mark so that this function is deterministic
			PlayerState->SetHasScored(true);
		}
	}

	return true;
}

void APaperGolfMatchGameState::DoAdditionalHoleComplete()
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: DoAdditionalHoleComplete"), *GetName());

	// Compute match scores - only gets called on server
	check(HasAuthority());

	FGolfMatchPlayerStateArray MatchPlayerStates = GetGolfMatchStatePlayerArray();

	if (MatchPlayerStates.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogPGPawn, Error, TEXT("%s: DoAdditionalHoleComplete - No MatchPlayerStates"), *GetName());
		return;
	}

	// all players should have the same number of completed holes
	struct FBestScore
	{
		AGolfMatchPlayerState* State;
		int32 Score;
	};

	TArray<FBestScore, TInlineAllocator<PG::MaxPlayers>> BestScores;
	BestScores.Reserve(MatchPlayerStates.Num());

	for (auto PlayerState : MatchPlayerStates)
	{
		// Shots is the final score for the hole and is valid until the next hole starts
		BestScores.Add({ PlayerState, PlayerState->GetShots() });
	}

	BestScores.Sort([](const FBestScore& A, const FBestScore& B)
	{
		return A.Score < B.Score;
	});

	// select the appropriate scoring criteria based on the number of players
	const auto& ScoringCriteria = GetScoreConfig(MatchPlayerStates.Num());
	const auto NumPointAwards = ScoringCriteria.Points.Num();

	if(!ensureMsgf(NumPointAwards > 0, TEXT("%s: DoAdditionalHoleComplete - ScoringCriteria has no entries for %d player%s"),
		*GetName(), MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num())))
	{
		UE_VLOG_UELOG(this, LogPGPawn, Error,
			TEXT("%s: DoAdditionalHoleComplete - DoAdditionalHoleComplete - No scoring criteria found for %d players"),
			*GetName(), MatchPlayerStates.Num(), LoggingUtils::Pluralize(MatchPlayerStates.Num()));
		return;
	}

	UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: DoAdditionalHoleComplete: NumPointAwards=%d; Scores=%s"),
		*GetName(), NumPointAwards,
		*PG::ToString(MatchPlayerStates, [](const AGolfMatchPlayerState* PlayerState) { return FString::Printf(TEXT("%d"), PlayerState->GetShots()); }));

	for (int32 AwardIndex = -1, TiedCount = 0, LastScore = std::numeric_limits<int32>::max(); const auto& Entry : BestScores)
	{
		if (Entry.Score == LastScore)
		{
			++TiedCount;
		}
		else
		{
			LastScore = Entry.Score;
			AwardIndex += TiedCount + 1;
			TiedCount = 0;
		}

		if (AwardIndex < NumPointAwards)
		{
			UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: DoAdditionalHoleComplete - Award %s %d points for #%d finish"), *GetName(),
				*Entry.State->GetName(), ScoringCriteria.Points[AwardIndex], AwardIndex + 1);

			Entry.State->AwardPoints(ScoringCriteria.Points[AwardIndex]);
		}
		else
		{
			// Awarded all the points but need to mark other players

			UE_VLOG_UELOG(this, LogPGPawn, Verbose, TEXT("%s: DoAdditionalHoleComplete - Award 0 points for #%d finish"), *GetName(),
				*Entry.State->GetName(), AwardIndex + 1);
			Entry.State->AwardPoints(0);
		}
	}
}

void APaperGolfMatchGameState::OnDisplayScoreUpdated(AGolfMatchPlayerState& PlayerState)
{
	UE_VLOG_UELOG(this, LogPGPawn, Log, TEXT("%s: OnDisplayScoreUpdated - PlayerState=%s"), *GetName(), *PlayerState.GetName());

	UpdatedMatchPlayerStates.AddUnique(&PlayerState);

	CheckScoreSyncState();
}
