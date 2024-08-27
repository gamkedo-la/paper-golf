// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "GolfAIConfigData.generated.h"

USTRUCT(BlueprintType)
struct FGolfAIConfigData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Shots Since Last Error")
	int32 ShotsSinceLastError{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Perfect Shot Probability")
	float PerfectShotProbability{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Shank Shot Probability")
	float ShankShotProbability{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Shank Accuracy")
	float ShankAccuracy{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Shank Power Factor")
	float ShankPowerFactor{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Accuracy Delta Min")
	float DefaultAccuracyDeltaMin{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Accuracy Delta Max")
	float DefaultAccuracyDeltaMax{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Power Delta Min")
	float DefaultPowerDeltaMin{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Power Delta Max")
	float DefaultPowerDeltaMax{};
};

namespace GolfAIConfigDataParser
{
	TArray<FGolfAIConfigData> ReadAll(UDataTable* GolfAIDataTable);
}