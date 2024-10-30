// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Config/GameSessionConfig.h"

#include "MultiplayerSessionsLogging.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSessionConfig)


const FString UGameSessionConfig::RandomMapName = TEXT("Random");

void UGameSessionConfig::Validate()
{
#if !UE_BUILD_SHIPPING
	ValidateMaps();
	ValidateGameModes();
#endif
}

bool UGameSessionConfig::IsValid() const
{
	if (!ensure(!Maps.IsEmpty()) || !ensure(!MatchTypesToModes.IsEmpty()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: Maps or MatchTypesToModes is empty: MapsSize=%d; MatchTypesToModesSize=%d"),
			*GetName(), Maps.Num(), MatchTypesToModes.Num());
		return false;
	}

	return true;
}

void UGameSessionConfig::ValidateMaps()
{
	for (auto It = Maps.CreateIterator(); It; ++It)
	{
		if (It->Value.IsNull())
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Invalid map in Maps array at key=%s"), *GetName(), *It->Key);
			It.RemoveCurrent();
		}
	}

	if (!ensureMsgf(!Maps.IsEmpty(), TEXT("%s: No valid maps defined!"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: No valid maps defined!"), *GetName());
	}
}

void UGameSessionConfig::ValidateGameModes()
{
	for (auto It = MatchTypesToModes.CreateIterator(); It; ++It)
	{
		if (It.Key().IsEmpty())
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Invalid matchType in MatchTypesToModes array"), *GetName());
			It.RemoveCurrent();
		}
		else if (!It.Value().IsValid())
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Invalid game mode in MatchTypesToModes array for MatchType=%s"), *GetName(), *It.Key());
			It.RemoveCurrent();
		}
	}

	if (!ensureMsgf(!MatchTypesToModes.IsEmpty(), TEXT("%s: No valid game mode mappings defined!"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: No valid game mode mappings defined!"), *GetName());
	}
}

const FGameModeInfo& UGameSessionConfig::GetGameModeInfo(const FString& MatchType, bool& bOutValid) const
{
	const static FGameModeInfo InvalidGameMode;

	const auto& FoundMatchTypeInfo = MatchTypesToModes.Find(MatchType);
	if (!FoundMatchTypeInfo)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: MatchType=%s not found in MatchTypesToMode"), *GetName(), *MatchType);
		bOutValid = false;
		return InvalidGameMode;
	}

	// Should have already been removed through validation
	bOutValid = FoundMatchTypeInfo->IsValid();
	check(bOutValid);

	return *FoundMatchTypeInfo;
}

TSoftObjectPtr<UWorld> UGameSessionConfig::GetMap(const FString& MapName) const
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: GetMap - %s"), *GetName(), *MapName);

	const TSoftObjectPtr<UWorld> MatchedMap{};

	if (MapName != RandomMapName)
	{
		if (auto FindResult = Maps.Find(MapName); FindResult)
		{
			return *FindResult;
		}
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetMap - MapName=%s not found in Maps"), *GetName(), *MapName);
	}

	return GetRandomMap();
}

TSoftObjectPtr<UWorld> UGameSessionConfig::GetRandomMap() const
{
	if (!ensureAlways(!Maps.IsEmpty()))
	{
		return nullptr;
	}

	auto MapIndex = FMath::RandRange(0, Maps.Num() - 1);

	for (auto It = Maps.CreateConstIterator(); It; ++It)
	{
		if (MapIndex == 0)
		{
			return It.Value();
		}
		--MapIndex;
	}

	checkNoEntry();

	return nullptr;
}

FString UGameSessionConfig::GetPathForMap(const TSoftObjectPtr<UWorld>& World) const
{
	// For some reason an odd extension is being added : e.g. House.house
	// remove the extension
	FString Path = World.ToSoftObjectPath().ToString();
	const auto Index = Path.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (Index == INDEX_NONE)
	{
		return Path;
	}

	return Path.Left(Index);
}

FString UGameSessionConfig::GetGameModePath(const TSoftClassPtr<AGameModeBase>& GameMode) const
{
	return GameMode.ToSoftObjectPath().ToString();
}

bool FGameModeInfo::IsValid() const
{
	return !Name.IsEmpty() && !GameMode.IsNull() && MinPlayers > 0;
}

TMap<FString, FString> UGameSessionConfig::GetMatchTypesToModeNames() const
{
	TMap<FString, FString> MatchTypesToNames;

	for (const auto& [MatchType, GameModeInfo] : MatchTypesToModes)
	{
		MatchTypesToNames.Add(MatchType, GameModeInfo.Name);
	}

	return MatchTypesToNames;
}


TArray<FGameModeInfo> UGameSessionConfig::GetAllGameModes() const
{
	TArray<FGameModeInfo> GameModes;
	MatchTypesToModes.GenerateValueArray(GameModes);
	return GameModes;
}

TArray<FString> UGameSessionConfig::GetAllMapNames() const
{
	TArray<FString> MapNames;
	Maps.GenerateKeyArray(MapNames);

	if (bEnableRandomMap)
	{
		MapNames.Add(RandomMapName);
	}

	return MapNames;
}