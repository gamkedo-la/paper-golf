// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "PGGameInstance.h"

#include "PaperGolfLogging.h"
#include "Logging/LoggingUtils.h"

#include "VisualLogger/VisualLogger.h"

#include "MoviePlayer.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PGGameInstance)

void UPGGameInstance::Init()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Init"), *GetName());

	Super::Init();

	InitLoadingScreen();
}

void UPGGameInstance::InitLoadingScreen()
{
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::EndLoadingScreen);
}

void UPGGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: BeginLoadingScreen: %s - Skip since on dedicated server"), *GetName(), *MapName);
		return;
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: BeginLoadingScreen: %s"), *GetName(), *MapName);

	DoLoadingScreen();
}

void UPGGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: EndLoadingScreen: %s"), *GetName(), *LoggingUtils::GetName(InLoadedWorld));
}

void UPGGameInstance::DoLoadingScreen()
{
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.bAllowInEarlyStartup = true;
	LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}
