// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Data/GolfAIConfigData.h"

#include "Utils/PGDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIConfigData)

TArray<FGolfAIConfigData> GolfAIConfigDataParser::ReadAll(UDataTable* GolfAIDataTable)
{
	return PG::DataTableUtils::ReadAll<FGolfAIConfigData>(GolfAIDataTable, true, "GolfAIConfigDataParser::ReadAll");
}
