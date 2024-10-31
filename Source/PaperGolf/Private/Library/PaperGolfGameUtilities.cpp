// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Library/PaperGolfGameUtilities.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/PlayerController.h"

#include "Logging/LoggingUtils.h"

#include "Logging/StructuredLog.h"

#include "PaperGolfLogging.h"

#include "Config/GameModeOptionParams.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperGolfGameUtilities)

namespace
{
	constexpr auto NextCourseKey = TEXT("next=");
	constexpr auto EncodedKey = TEXT('@');
	constexpr auto EncodedSeparator = TEXT('~');
}

APlayerController* UPaperGolfGameUtilities::GetLocalPlayerController(UObject* WorldContextObject)
{
	auto GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetFirstLocalPlayerController(WorldContextObject ? WorldContextObject->GetWorld() : nullptr);
}

APawn* UPaperGolfGameUtilities::GetLocalPlayer(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->GetPawn();
}

APlayerCameraManager* UPaperGolfGameUtilities::GetLocalPlayerCameraManager(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->PlayerCameraManager;
}

APlayerState* UPaperGolfGameUtilities::GetLocalPlayerState(UObject* WorldContextObject)
{
	auto PlayerController = GetLocalPlayerController(WorldContextObject);
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->PlayerState;
}

FString UPaperGolfGameUtilities::GetCurrentMapName(UObject* WorldContextObject)
{
	if (!ensure(WorldContextObject))
	{
		return {};
	}

	const auto World = WorldContextObject->GetWorld();
	if (!ensure(World))
	{
		return {};
	}

	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix); // Remove any prefix if necessary

	return MapName;
}

FString UPaperGolfGameUtilities::CreateCourseOptionsUrl(const FString& Map,
	const FString& GameMode, int32 NumPlayers, int32 NumBots, int32 AllowBots, int32 MaxPlayers)
{
	FString Options = FString::Printf(TEXT("%s?game=%s?%s%d?%s%d"),
		*Map, *GameMode,
		PG::GameModeOptions::NumPlayers, NumPlayers,
		PG::GameModeOptions::NumBots, NumBots
	);

	if (AllowBots >= 0)
	{
		Options += '?';
		Options += PG::GameModeOptions::AllowBots;
		Options.AppendInt(AllowBots);
	}

	if (MaxPlayers > 0)
	{
		Options += '?';
		Options += PG::GameModeOptions::MaxPlayers;
		Options.AppendInt(MaxPlayers);
	}

	UE_LOGFMT(LogPaperGolfGame, Log,
		"CreateCourseOptionsUrl: Map={0}; GameMode={1}; NumPlayers={2}; NumBots={3}; AllowBots={4}; MaxPlayers={5} -> Url={6}",
		*Map, *GameMode, NumPlayers, NumBots, AllowBots, MaxPlayers, *Options);

	return Options;
}

void UPaperGolfGameUtilities::EncodeNextCourseOptionsInline(FString NextCourseOptions, FString& Options)
{
	Options += '?';
	Options += NextCourseKey;

	NextCourseOptions.ReplaceCharInline(TEXT('?'), EncodedKey);
	NextCourseOptions.ReplaceCharInline(TEXT('='), EncodedSeparator);
	
	Options += NextCourseOptions;

	UE_LOGFMT(LogPaperGolfGame, Log,
		"EncodeNextCourseOptions: EncodedNextCourseOptions={0}; Options={1}",
		*NextCourseOptions, *Options);
}

FString UPaperGolfGameUtilities::DecodeNextCourseOptions(const FString& Options)
{
	FString NextCourseOptions;
	if (!FParse::Value(*Options, NextCourseKey, NextCourseOptions))
	{
		return {};
	}

	// Decode the course options

	NextCourseOptions.ReplaceCharInline(EncodedKey, '?');
	NextCourseOptions.ReplaceCharInline(EncodedSeparator, '=');

	UE_LOGFMT(LogPaperGolfGame, Log,
		"DecodeNextCourseOptions: Options={0}; NextCourseOptions={1}",
		*Options, *NextCourseOptions);

	return NextCourseOptions;
}
