// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Config/BotNameConfig.h"

#include "Utils/PGDataTableUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BotNameConfig)


TArray<FString> PG::BotNameParser::ReadAll(UDataTable* BotNameDataTable)
{
	const auto BotNameConfigs = PG::DataTableUtils::ReadAll<FBotNameConfig>(BotNameDataTable, true, "BotNameParser");
	TArray<FString> BotNames;
	BotNames.Reserve(BotNameConfigs.Num());

	for (const auto& BotNameConfig : BotNameConfigs)
	{
		BotNames.Add(BotNameConfig.Name);
	}

	return BotNames;
}
