// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataTable.h"

#include "BotNameConfig.generated.h"

/**
 * Names for bots.
 */
USTRUCT(BlueprintType)
struct FBotNameConfig : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Name")
	FString Name{};
};

namespace PG::BotNameParser
{
	TArray<FString> ReadAll(UDataTable* BotNameDataTable);
}
