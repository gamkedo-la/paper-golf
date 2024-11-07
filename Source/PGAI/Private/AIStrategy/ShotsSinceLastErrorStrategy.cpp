// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "AIStrategy/ShotsSinceLastErrorStrategy.h"

#include "Utils/RandUtils.h"
#include "Utils/GolfAIShotCalculationUtils.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotsSinceLastErrorStrategy)

using namespace PG;

bool UShotsSinceLastErrorStrategy::Initialize(const PG::FAIPerformanceConfig& Config)
{
	bool bValid = Super::Initialize(Config);

	AIErrorsData = GolfAIConfigDataParser::ReadAll(AIErrorsDataTable);
	if (AIErrorsData.IsEmpty())
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Error, TEXT("%s-%s: ValidateConfig - FALSE - AIErrorsData is empty"), *LoggingUtils::GetName(GetOuter()), *GetName());
		bValid = false;
	}

	return bValid;
}

PG::FShotErrorResult UShotsSinceLastErrorStrategy::CalculateShotError(const FFlickParams& FlickParams)
{
	const auto AIConfigEntry = SelectAIConfigEntry();
	if (!AIConfigEntry)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: CalculateShotError - AIConfigEntry is NULL - using defaults"),
			*LoggingUtils::GetName(GetOuter()), *GetName());

		return Super::CalculateShotError(FlickParams);
	}

	auto PowerFraction = FlickParams.PowerFraction;

	const auto PerfectShotRoll = FMath::FRand();

	if (PerfectShotRoll <= AIConfigEntry->PerfectShotProbability)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Perfect Shot - PerfectShotRoll=%.2f; PerfectShotProbability=%.2f; ShotsSinceLastError=%d"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), PerfectShotRoll, AIConfigEntry->PerfectShotProbability, ShotsSinceLastError);

		++ShotsSinceLastError;

		return FShotErrorResult
		{
			.PowerFraction = PowerFraction,
			.Accuracy = 0.0f
		};
	}

	ShotsSinceLastError = 0;

	// Check for a "shank shot"
	const auto ShankShotRoll = FMath::FRand();

	if (ShankShotRoll <= AIConfigEntry->ShankShotProbability)
	{
		UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Shank Shot - ShankShotRoll=%.2f; ShankShotProbability=%.2f"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), ShankShotRoll, AIConfigEntry->ShankShotProbability);

		const auto ShankPowerFactor = AIConfigEntry->ShankPowerFactor;
		if (ShankPowerFactor * PowerFraction <= 1.0f)
		{
			PowerFraction *= ShankPowerFactor;
		}
		else
		{
			PowerFraction = FMath::Max(AIConfig.MinPower, PowerFraction / ShankPowerFactor);
		}

		const auto Accuracy = RandUtils::RandSign() * AIConfigEntry->ShankAccuracy;

		UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Shank Shot - PowerFraction=%.2f; ShankPowerFactor=%.2f; ShankAccuracy=%.2f"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), PowerFraction, ShankPowerFactor, Accuracy);

		return FShotErrorResult
		{
			.PowerFraction = PowerFraction,
			.Accuracy = Accuracy
		};
	}

	const auto Accuracy = GolfAIShotCalculationUtils::GenerateAccuracy(AIConfigEntry->DefaultAccuracyDeltaMin, AIConfigEntry->DefaultAccuracyDeltaMax);
	PowerFraction = GolfAIShotCalculationUtils::GeneratePowerFraction(PowerFraction, AIConfig.MinPower, AIConfigEntry->DefaultPowerDeltaMin, AIConfigEntry->DefaultPowerDeltaMax);

	// use default calculation
	UE_VLOG_UELOG(GetOuter(), LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Default - PowerFraction=%.2f; Accuracy = %.2f; AccuracyDeviation=[%.2f,%.2f]; PowerDeviation=[%.2f,%.2f]"),
		*LoggingUtils::GetName(GetOuter()), *GetName(),
		PowerFraction, Accuracy, AIConfigEntry->DefaultAccuracyDeltaMin, AIConfigEntry->DefaultAccuracyDeltaMax, AIConfigEntry->DefaultPowerDeltaMin, AIConfigEntry->DefaultPowerDeltaMax);

	return FShotErrorResult
	{
		.PowerFraction = PowerFraction,
		.Accuracy = Accuracy,
	};
}

const FGolfAIConfigData* UShotsSinceLastErrorStrategy::SelectAIConfigEntry() const
{
	const FGolfAIConfigData* Selected = AIErrorsData.FindByPredicate([this](const FGolfAIConfigData& Entry)
		{
			return ShotsSinceLastError <= Entry.ShotsSinceLastError;
		});

	if (!Selected && !AIErrorsData.IsEmpty())
	{
		Selected = &AIErrorsData.Last();

		UE_VLOG_UELOG(GetOuter(), LogPGAI, Warning, TEXT("%s-%s: SelectAIConfigEntry - ShotsSinceLastError=%d; Defaulted to last entry with Shots=%d"),
			*LoggingUtils::GetName(GetOuter()), *GetName(), ShotsSinceLastError, Selected->ShotsSinceLastError);
	}

	return Selected;
}
