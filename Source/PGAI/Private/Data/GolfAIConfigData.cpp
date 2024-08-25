// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Data/GolfAIConfigData.h"

#include "Utils/PGDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIConfigData)

TArray<FGolfAIConfigData> GolfAIConfigDataParser::ReadAll(UDataTable* GolfAIDataTable)
{
	if (!PG::DataTableUtils::ValidateDataTableRowType<FGolfAIConfigData>(GolfAIDataTable))
	{
		return {};
	}

	TArray<FGolfAIConfigData*> Data;
	GolfAIDataTable->GetAllRows("GolfAIConfigDataParser::ReadAll", Data);

	TArray<FGolfAIConfigData> OutputData;
	OutputData.Reserve(Data.Num());

	for (auto DataRowPtr : Data)
	{
		if (DataRowPtr)
		{
			OutputData.Add(*DataRowPtr);
		}
	}

	return OutputData;
}
