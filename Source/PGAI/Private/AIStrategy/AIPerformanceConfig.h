// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PaperGolfTypes.h"

#include "AIPerformanceConfig.generated.h"

USTRUCT(BlueprintType)
struct FAIPerformanceConfigData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Weight")
	float RowWeight{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Accuracy Delta Min")
	float AccuracyDeltaMin{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Accuracy Delta Max")
	float AccuracyDeltaMax{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Power Delta Min")
	float PowerDeltaMin{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Default Power Delta Max")
	float PowerDeltaMax{};
};

namespace AIPerformanceConfigDataTableParser
{
	TArray<FAIPerformanceConfigData> ReadAll(UDataTable* GolfAIDataTable);
}
