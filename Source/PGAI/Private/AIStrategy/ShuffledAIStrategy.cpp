// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "AIStrategy/ShuffledAIStrategy.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"

#include "Utils/RandUtils.h"
#include "Utils/GolfAIShotCalculationUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShuffledAIStrategy)

using namespace PG;

bool UShuffledAIStrategy::Initialize(const PG::FAIPerformanceConfig& Config)
{
	bool bValid = Super::Initialize(Config);
	
	if (!InitializeShotTypeStrategyData())
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Error, TEXT("%s-%s: Initialize - FALSE - InitializeShotTypeStrategyData failed"), *LoggingUtils::GetName(GetOuter()), *GetName());
		bValid = false;
	}

	return bValid;
}

FShotErrorResult UShuffledAIStrategy::CalculateShotError(const FFlickParams& FlickParams)
{
	const FAIPerformanceConfigData* ConfigEntry = GetNextConfigEntry(FlickParams.ShotType);

	if (!ConfigEntry)
	{
		return Super::CalculateShotError(FlickParams);
	}

	const auto Accuracy = GolfAIShotCalculationUtils::GenerateAccuracy(ConfigEntry->AccuracyDeltaMin, ConfigEntry->AccuracyDeltaMax);
	const auto PowerFraction = GolfAIShotCalculationUtils::GeneratePowerFraction(FlickParams.PowerFraction, FMath::Max(AIConfig.MinPower, ConfigEntry->PowerAbsoluteMin),
		ConfigEntry->PowerDeltaMin, ConfigEntry->PowerDeltaMax, ConfigEntry->PowerSignBias);

	UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CalculateShotError - PowerFraction=%.2f; Accuracy = %.2f; AccuracyDeviation=[%.2f,%.2f]; PowerDeviation=[%.2f,%.2f]"),
		*LoggingUtils::GetName(GetOuter()), *GetName(),
		PowerFraction, Accuracy, ConfigEntry->AccuracyDeltaMin, ConfigEntry->AccuracyDeltaMax, ConfigEntry->PowerDeltaMin, ConfigEntry->PowerDeltaMax);

	return FShotErrorResult
	{
		.PowerFraction = PowerFraction,
		.Accuracy = Accuracy,
	};
}

bool UShuffledAIStrategy::InitializeShotTypeStrategyData()
{
	bool bValid = !ShotTypeStrategyTableMap.IsEmpty();

	for (const auto& [ShotType, ShotTypeConfig] : ShotTypeStrategyTableMap)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: InitializeShotTypeStrategyData - ShotType=%s; ShotTypeDataTable=%s"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType), *LoggingUtils::GetName(ShotTypeConfig.DataTable));

		auto TableDataRows = CreateShotTypeEntries(ShotType, ShotTypeConfig);
		if (!TableDataRows.IsEmpty())
		{
			ShotTypeStrategyDataMap.Add(ShotType, TableDataRows);
			ShotTypeStrategyIndexMap.Add(ShotType, 0);
		}
		else
		{
			UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: InitializeShotTypeStrategyData: ShotType=%s - TableDataRows is empty - ignoring entry"),
				*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType));

			bValid = false;
		}
	}

	return bValid;
}

TArray<FAIPerformanceConfigData> UShuffledAIStrategy::CreateShotTypeEntries(EShotType ShotType, const FShuffleAIStrategyShotTypeConfig& ConfigEntry) const
{
	auto TableRows = AIPerformanceConfigDataTableParser::ReadAll(ConfigEntry.DataTable);
	
	if (TableRows.IsEmpty())
	{
		return TableRows;
	}

	if (TableRows.Num() > ConfigEntry.NumOutcomes)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: CreateShotTypeEntries - More entries than desired outcomes! ShotType=%s; ShotTypeDataTable=%s; TableRows.Num()=%d; DesiredNumOutcomes=%d"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType), *LoggingUtils::GetName(ConfigEntry.DataTable), TableRows.Num(), ConfigEntry.NumOutcomes);
	}

	// Determine the weight sum of all the entries
	double WeightSum{};
	for (const auto& Row : TableRows)
	{
		WeightSum += Row.RowWeight;
	}

	const auto NormalizedRowWeightFactor = ConfigEntry.NumOutcomes / WeightSum;

	TArray<FAIPerformanceConfigData> FinalEntries;
	FinalEntries.Reserve(ConfigEntry.NumOutcomes);

	// Sort by row weight so that last entry gets the most entries
	TableRows.Sort([](const FAIPerformanceConfigData& A, const FAIPerformanceConfigData& B)
	{
		return A.RowWeight < B.RowWeight;
	});

	// Normalize the weights
	for (int32 i = 0; auto& Row : TableRows)
	{
		const auto NormalizedRowWeight = Row.RowWeight * NormalizedRowWeightFactor;

		int32 NumEntries{};

		// Last entry takes up the remainder
		if (i < TableRows.Num() - 1)
		{
			NumEntries = FMath::Fractional(NormalizedRowWeight) >= ConfigEntry.RoundUpFraction ? 
				FMath::CeilToInt(NormalizedRowWeight) : FMath::FloorToInt(NormalizedRowWeight);

		}
		// Always round up or take the remainder of desired outcomes on last entry
		else
		{
			NumEntries = FMath::Max(ConfigEntry.NumOutcomes - FinalEntries.Num(), FMath::CeilToInt(NormalizedRowWeight));
		}

		if (NumEntries > 0)
		{
			for (int32 j = 0; j < NumEntries; ++j)
			{
				FinalEntries.Add(Row);
			}

			UE_VLOG_UELOG(GetOuter(), LogPGAI, Verbose, TEXT("%s-%s: CreateShotTypeEntries - i=%d - NumEntries=%d;  Entry=%s; NormalizedRowWeight=%.2f; ConfigEntry.NumOutcomes=%d; FinalEntries.Num()=%d"),
				*LoggingUtils::GetName(GetOuter()), *GetName(), i, NumEntries, *Row.ToString(), NormalizedRowWeight, ConfigEntry.NumOutcomes, FinalEntries.Num());
		}
		else if (NumEntries == 0)
		{
			UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CreateShotTypeEntries - i=%d - Entry=%s was truncated. NormalizedRowWeight=%.2f; ConfigEntry.NumOutcomes=%d; FinalEntries.Num()=%d"),
				*LoggingUtils::GetName(GetOuter()), *GetName(), i, *Row.ToString(), NormalizedRowWeight, ConfigEntry.NumOutcomes, FinalEntries.Num());
		}
		else
		{
			UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: CreateShotTypeEntries - i=%d - Entry=%s; NumEntries=%d < 0! NormalizedRowWeight=%.2f; ConfigEntry.NumOutcomes=%d; FinalEntries.Num()=%d"),
				*LoggingUtils::GetName(GetOuter()), *GetName(), i, *Row.ToString(), NumEntries, NormalizedRowWeight, ConfigEntry.NumOutcomes, FinalEntries.Num());
		}

		++i;
	} // for

	UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CreateShotTypeEntries - ShotType=%s; ShotTypeDataTable=%s; FinalEntries.Num()=%d; DesiredNumOutcomes=%d; TableRows.Num()=%d"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType), *LoggingUtils::GetName(ConfigEntry.DataTable),
		FinalEntries.Num(), ConfigEntry.NumOutcomes, TableRows.Num());

	ShuffleEntries(FinalEntries);

	return FinalEntries;
}

const FAIPerformanceConfigData* UShuffledAIStrategy::GetNextConfigEntry(EShotType ShotType)
{
	TArray<FAIPerformanceConfigData>* Entries = ShotTypeStrategyDataMap.Find(ShotType);
	if (!Entries)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: GetNextConfigEntry - ShotType=%s - No entries found - will use default error"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType));
		return nullptr;
	}

	check(!Entries->IsEmpty());
	
	int32* IndexPtr = ShotTypeStrategyIndexMap.Find(ShotType);
	check(IndexPtr);

	const FAIPerformanceConfigData* Entry = &(*Entries)[*IndexPtr];

	UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: GetNextConfigEntry - ShotType=%s; Index=%d; Entry=%s"),
		*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType), *IndexPtr, *Entry->ToString());

	// Prepare for next call
	const auto NextIndex = ++(*IndexPtr);

	if (NextIndex >= Entries->Num())
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: GetNextConfigEntry - ShotType=%s reached end of %d entries - shuffling and setting next call back to 0"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), *LoggingUtils::GetName(ShotType), Entries->Num());

		ShuffleEntries(*Entries);
		*IndexPtr = 0;
	}

	return Entry;
}

void UShuffledAIStrategy::ShuffleEntries(TArray<FAIPerformanceConfigData>& Entries) const
{
	RandUtils::ShuffleArray(Entries);
}
