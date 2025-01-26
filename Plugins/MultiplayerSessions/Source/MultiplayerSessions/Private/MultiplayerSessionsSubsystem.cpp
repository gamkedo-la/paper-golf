// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "MultiplayerSessionsLogging.h"

#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h" 
#include "Interfaces/OnlineIdentityInterface.h"
#include "GameFramework/PlayerState.h"
#include "VisualLogger/VisualLogger.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

#include <compare>

#include UE_INLINE_GENERATED_CPP_BY_NAME(MultiplayerSessionsSubsystem)

namespace
{
	const FName BuildIdSettingName = "PGBuildId";
}

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
{
}

void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Initialize"), *GetName());

	Super::Initialize(Collection);
}

void UMultiplayerSessionsSubsystem::Configure(const FSessionsConfiguration& InConfiguration)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Configure: bIsLanMatch=%s; bClearAllState=%s"),
		*GetName(),
		InConfiguration.bIsLanMatch ? TEXT("TRUE") : TEXT("FALSE"),
		InConfiguration.bClearAllState ? TEXT("TRUE") : TEXT("FALSE")
	);

#if WITH_EDITOR
	const FName& DesiredSubsystem = GIsEditor || InConfiguration.bIsLanMatch ? NULL_SUBSYSTEM : STEAM_SUBSYSTEM;
#else
	const FName& DesiredSubsystem = InConfiguration.bIsLanMatch ? NULL_SUBSYSTEM : STEAM_SUBSYSTEM;
#endif

	const bool bDestroyPreviousSubsystem = InConfiguration.bClearAllState || DesiredSubsystem != LastSubsystemName;


	// Be sure we first close any previous sessions that may still be open
	ForceDestroyExistingSession();

	// If we are switching subsystems based on LAN flag or need to completely clear out any previous partial state then we need to deinitialize the current subsystem
	if (bDestroyPreviousSubsystem)
	{
		DestroyOnlineSubsystem();
	}

	// Must do this before IOnlineSubsystem::Get; otherwise, it will return NULL if not enabled in the config ini
	SetSubsystemEnabled(DesiredSubsystem, true);
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(DesiredSubsystem);

	if (!OnlineSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Configure - Preferred subsystem=%s OnlineSubsystem is NULL"), *GetName(), *DesiredSubsystem.ToString());
		return;
	}

	if (!OnlineSubsystem->IsEnabled())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Configure - Subsystem %s could not be enabled"), *GetName(), *DesiredSubsystem.ToString());
		return;
	}

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: Configure: DesiredSubsystem=%s and using subsystem %s"), *GetName(), *DesiredSubsystem.ToString(), *OnlineSubsystem->GetSubsystemName().ToString());

	OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
	SessionsConfiguration = InConfiguration;
	LastSubsystemName = DesiredSubsystem;
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Deinitialize"), *GetName());

	DestroyOnlineSubsystem();

	Super::Deinitialize();
}

void UMultiplayerSessionsSubsystem::DestroyOnlineSubsystem()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: DestroyOnlineSubsystem=%s; OnlineSessionInterface valid = %s"),
		*GetName(), *LastSubsystemName.ToString(), OnlineSessionInterface.IsValid() ? TEXT("TRUE") : TEXT("FALSE"));

	if (!OnlineSessionInterface.IsValid() && LastSubsystemName.IsNone())
	{
		return;
	}

	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	OnlineSessionInterface.Reset();

	if (!LastSubsystemName.IsNone())
	{
		SetSubsystemEnabled(LastSubsystemName, false);
		//IOnlineSubsystem::Destroy(LastSubsystemName);
		LastSubsystemName = NAME_None;
	}
}

void UMultiplayerSessionsSubsystem::SetSubsystemEnabled(const FName& SubsystemName, bool bIsEnabled)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: SetSubsystemEnabled: SubsystemName=%s; bIsEnabled=%s"), *GetName(), *SubsystemName.ToString(), bIsEnabled ? TEXT("TRUE") : TEXT("FALSE"));

	// Get the config entry and toggle it  - see OnlineSubsystem.cpp Line 407 for why we need to do this - as otherwise if the entry is bEnabled false then it won't load it with GetSubsystem
	const FString ConfigSection(FString::Printf(TEXT("OnlineSubsystem%s"), *SubsystemName.ToString()));

	// Set the subsystem enabled status - will only work if the section is present; otherwise, will silently fail
	// If the section isn't present then it will be enabled by default per the OnlineSubsystem.cpp logic
	if (ensure(GConfig))
	{
		GConfig->SetBool(*ConfigSection, TEXT("bEnabled"), bIsEnabled, GEngineIni);
	}
}

TArray<FOnlineSessionSearchResult> UMultiplayerSessionsSubsystem::FilterSessionSearchResults(const TArray<FOnlineSessionSearchResult>& Results) const
{
	TArray<FOnlineSessionSearchResult> FilteredResults;

	for ([[maybe_unused]] int32 Index = 0; const auto& Result : Results)
	{
		FString SettingsValue;
		const bool FoundMatchType = Result.Session.SessionSettings.Get(UMultiplayerSessionsSubsystem::SessionMatchTypeName, SettingsValue);

		if (!FoundMatchType)
		{
			continue;
		}

		// Match build id
		if (BuildId)
		{
			FString BuildIdValue;
			const bool FoundBuildId = Result.Session.SessionSettings.Get(BuildIdSettingName, BuildIdValue);

			if (!FoundBuildId || FCString::Atoi(*BuildIdValue) != BuildId)
			{
				UE_VLOG_UELOG(this, LogMultiplayerSessions, Verbose,
					TEXT("%s: FilterSessionSearchResults - Skipping %d/%d due to BuildId mismatch: BuildId=%d; BuildIdValue=%s"),
					*GetName(), Index + 1, Results.Num(), BuildId, *BuildIdValue);
				continue;
			}
		}

		// Optionally filter out sessions that are full
		if (!SessionsConfiguration.bAllowJoinFullSessions && Result.Session.NumOpenPublicConnections <= 0)
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Verbose,
				TEXT("%s: FilterSessionSearchResults - Skipping %d/%d due to full session: NumOpenPublicConnections=%d"),
				*GetName(), Index + 1, Results.Num(), Result.Session.NumOpenPublicConnections);
			continue;
		}

		FilteredResults.Add(Result);
	}

	// Sort the results by ping in 10ms increments and then open connections descending
	FilteredResults.Sort([](const FOnlineSessionSearchResult& First, const FOnlineSessionSearchResult& Second)
	{
		const auto NormalizePing = [](int32 PingInMs)
		{
			return PingInMs / 10;
		};

		const auto FirstPing = NormalizePing(First.PingInMs);
		const auto SecondPing = NormalizePing(Second.PingInMs);

		const auto PingCompareResult = FirstPing <=> SecondPing;
		if (PingCompareResult != 0)
		{
			return PingCompareResult < 0;
		}

		return First.Session.NumOpenPublicConnections > Second.Session.NumOpenPublicConnections;
	});

	return FilteredResults;
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, const FString& MatchType, const FString& Map)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: CreateSession: NumPublicConnections=%d; MatchType=%s; Map=%s"), *GetName(), NumPublicConnections, *MatchType, *Map);

	if (!OnlineSessionInterface.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: CreateSession - OnlineSessionInterface is not valid: NumPublicConnections=%d; MatchType=%s; Map=%s"), 
			*GetName(), NumPublicConnections, *MatchType, *Map);

		MultiplayerOnCreateSessionComplete.Broadcast(false);

		return;
	}

	const auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Destroying existing NamedSession: NAME_GameSession"), *GetName());

		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		LastMap = Map;
		bCreateSessionOnDestroy = true;

		DestroySession();
		
		return;
	}

	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;
	DesiredMap = Map;

	// Call our callback function when the session is created
	// Store the delegate in a FDelegateHandle so we can later remove it from the delegaet list in OnlineSessionInterface
	CreateSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->bIsLANMatch = IsLanMatch();
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true; // Steam with presence - based on regions closeby - needed to work
	LastSessionSettings->bShouldAdvertise = true; // allow players to find sessions
	LastSessionSettings->bUsesPresence = true; // Allow us to use presence to find sessions in our region of the world
	LastSessionSettings->bUseLobbiesIfAvailable = true; // If cannot find sessions - look for lobbies
	LastSessionSettings->bAllowInvites = true; // Allow invites to the session
	LastSessionSettings->bIsDedicated = IsRunningDedicatedServer();

	LastSessionSettings->Set(SessionMatchTypeName, MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (BuildId)
	{
		LastSessionSettings->Set(BuildIdSettingName, BuildId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		LastSessionSettings->BuildUniqueId = BuildId; // So that we can join other sessions
	}

	// Get networkd id of first local player
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: CreateGameSession - LocalPlayer not found!"), *GetName());
		return;
	}

	const auto bWasSuccessful = OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings);

	if (!bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: CreateSession FAILED: NumPublicConnections=%d; MatchType=%s; Map=%s"), *GetName(), NumPublicConnections, *MatchType, *Map);

		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();

		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: CreateSession InProgress: NumPublicConnections=%d; MatchType=%s; Map=%s"), *GetName(), NumPublicConnections, *MatchType, *Map);

		// Don't call MultiplayerOnCreateSessionComplete.Broadcast(true) yet as we need to wait for the OnCreateSessionComplete function to trigger
	}
}

void UMultiplayerSessionsSubsystem::CreateLocalSession(int32 NumPublicConnections, const FString& MatchType, const FString& Map)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: CreateLocalSession: NumPublicConnections=%d; MatchType=%s; Map=%s"), *GetName(), NumPublicConnections, *MatchType, *Map);

	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;
	DesiredMap = Map;

	MultiplayerOnCreateSessionComplete.Broadcast(true);
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: FindSessions: MaxSearchResults=%d"), *GetName(), MaxSearchResults);

	if (!OnlineSessionInterface.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: FindSessions: MaxSearchResults=%d - OnlineSessionInterface is not valid"), *GetName(), MaxSearchResults);

		MultiplayerOnFindSessionsComplete.Broadcast({}, false);

		return;
	}

	FindSessionsCompleteDelegateHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = IsLanMatch();
	if (!LastSessionSearch->bIsLanQuery)
	{
		LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	}

	// Get networkd id of first local player
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	const bool bWasSuccessful = OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());

	if (!bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: FindSessions - FAILED: MaxSearchResults=%d"), *GetName(), MaxSearchResults);

		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		MultiplayerOnFindSessionsComplete.Broadcast({}, false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& OnlineSessionSearchResult)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: JoinSession: OnlineSessionSearchResult=%s"), *GetName(), *OnlineSessionSearchResult.GetSessionIdStr());

	if (!OnlineSessionInterface.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: JoinSession - OnlineSessionInterface is not valid"), *GetName());

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	if (!OnlineSessionSearchResult.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: JoinSession: OnlineSessionSearchResult is not valid"), *GetName());

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);

		return;
	}

	// Get network id of first local player
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_VLOG_UELOG(this, LogTemp, Error, TEXT("%s: JoinSession - LocalPlayer not found!"), *GetName());

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);

		return;
	}

	// Register callback when session is joined
	JoinSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	const bool bJoinWasSuccessful = OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, OnlineSessionSearchResult);

	if (!bJoinWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: JoinSession FAILED: OnlineSessionSearchResult=%s"), *GetName(), *OnlineSessionSearchResult.GetSessionIdStr());

		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

bool UMultiplayerSessionsSubsystem::GetOnlineUserName(AController* Controller, FString& UserName) const
{
	// If using Subsystem NULL then this information is not available
	if (LastSubsystemName.IsNone() || LastSubsystemName == NULL_SUBSYSTEM)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: GetOnlineUserName - FALSE - Controller=%s: Using Subsystem NULL - info not available - returning false"),
			*GetName(), Controller ? *Controller->GetName() : TEXT("NULL"));
		return false;
	}

	const auto PC = Cast<APlayerController>(Controller);
	if (!IsValid(PC))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: GetOnlineUserName - FALSE - Controller=%s is not a PlayerController - returning false"), *GetName(),
			Controller ? *Controller->GetName() : TEXT("NULL"));
		return false;

	}

	if (!IsValid(PC->PlayerState))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: GetOnlineUserName - FALSE - Controller=%s - PlayerState is not valid - returning false"), *GetName(),
			*Controller->GetName());
		return false;
	}

	const auto CurrentOnlineSub = IOnlineSubsystem::Get(LastSubsystemName);
	if (!CurrentOnlineSub)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetOnlineUserName - FALSE - IOnlineSubsystem::Get() returned NULL - returning false"), *GetName());
		return false;
	}

	const IOnlineIdentityPtr Identity = CurrentOnlineSub->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetOnlineUserName - FALSE - IOnlineSubsystem::GetIdentityInterface() returned NULL for Subsystem=%s - returning false"),
			*GetName(), *CurrentOnlineSub->GetSubsystemName().ToString());
		return false;
	}

	const FUniqueNetIdRepl& UserId = PC->PlayerState->GetUniqueId();

	if (!UserId.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetOnlineUserName - FALSE - (%s) Controller=%s; PlayerState=%s - UserId is not valid - returning false"),
			*GetName(), *LastSubsystemName.ToString(), *PC->GetName(), *PC->PlayerState->GetName());
		return false;
	}

	const auto& UniqueNetId = UserId.GetUniqueNetId();
	if (!UniqueNetId.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetOnlineUserName - FALSE - (%s) Controller=%s; PlayerState=%s - UniqueNetId is not valid - returning false"),
			*GetName(), *LastSubsystemName.ToString(), *PC->GetName(), *PC->PlayerState->GetName());
		return false;
	}

	// If you pass an index like 0-4 it gets the username of the local connected player controller
	UserName = Identity->GetPlayerNickname(*UniqueNetId);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: GetOnlineUserName - %s - (%s) Controller=%s; PlayerState=%s - UserName=%s"),
		*GetName(), *LastSubsystemName.ToString(), UserName.IsEmpty() ? TEXT("FALSE") : TEXT("TRUE"), *PC->GetName(), *PC->PlayerState->GetName(), *UserName);

	return !UserName.IsEmpty();
}

void UMultiplayerSessionsSubsystem::StartSession()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: StartSession"), *GetName());

	const auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (!ExistingSession)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: Start Session NAME_GameSession no longer valid - aborting"), *GetName());

		MultiplayerOnStartSessionComplete.Broadcast(false);

		DestroySession();
		return;
	}

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: StartSession - Starting session: %s"), *GetName(), *ExistingSession->SessionName.ToString());

	StartSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	const bool bStartWasSuccessful = OnlineSessionInterface->StartSession(ExistingSession->SessionName);

	if (!bStartWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: StartSession - FAILED"), *GetName());

		OnlineSessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: StartSession COMPLETED: SessionName=%s"), *GetName(), *SessionName.ToString());
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: StartSession FAILED in OnCreateSessionComplete: SessionName=%s"), *GetName(), *SessionName.ToString());
	}

	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	StartSessionCompleteDelegateHandle.Reset();

	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: DestroySession"), *GetName());

	if (!OnlineSessionInterface.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: DestroySession - OnlineSessionInterface is not valid"), *GetName());

		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	const auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: DestroySession - Destroying session NAME_GameSession: %s"),
		*GetName(), ExistingSession ? *ExistingSession->SessionName.ToString() : TEXT("NULL"));

	DestroySessionCompleteDelegateHandle = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	const bool bDestroyWasSuccessful = OnlineSessionInterface->DestroySession(NAME_GameSession);

	if (!bDestroyWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: DestroySession - FAILED"), *GetName());

		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

bool UMultiplayerSessionsSubsystem::IsSessionActive() const
{
	if (!OnlineSessionInterface.IsValid())
	{
		return false;
	}

	return OnlineSessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

void UMultiplayerSessionsSubsystem::ForceDestroyExistingSession()
{
	if (!IsSessionActive())
	{
		return;
	}

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: ForceDestroyExistingSession - Destroying session NAME_GameSession for Subsystem=%s"),
		*GetName(), *LastSubsystemName.ToString());

	check(OnlineSessionInterface.IsValid());

	// Destroy without invoking any callbacks
	OnlineSessionInterface->DestroySession(NAME_GameSession);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: DestroySession COMPLETED: SessionName=%s"), *GetName(), *SessionName.ToString());
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: DestroySession FAILED in OnCreateSessionComplete: SessionName=%s"), *GetName(), *SessionName.ToString());
	}

	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	DestroySessionCompleteDelegateHandle.Reset();

	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);

	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: DestroySession - requested create session on destroy: NumPublicConnections=%d; MatchType=%s"),
			*GetName(), LastNumPublicConnections, *LastMatchType);

		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType, LastMap);
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: CreateSession COMPLETED: SessionName=%s"), *GetName(), *SessionName.ToString());
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: CreateSession FAILED in OnCreateSessionComplete: SessionName=%s"), *GetName(), *SessionName.ToString());
	}

	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	CreateSessionCompleteDelegateHandle.Reset();

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	TArray<FOnlineSessionSearchResult> SearchResults;

	if (bWasSuccessful)
	{
		check(LastSessionSearch);

		SearchResults = FilterSessionSearchResults(LastSessionSearch->SearchResults);

		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: OnFindSessions COMPLETED: RawSearchResultsFound=%d; EligibleSearchResultsFound=%d"),
			*GetName(), LastSessionSearch->SearchResults.Num(), SearchResults.Num());
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnFindSessions FAILED in OnCreateSessionComplete"), *GetName());
	}

	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	FindSessionsCompleteDelegateHandle.Reset();

	if (SearchResults.IsEmpty())
	{
		MultiplayerOnFindSessionsComplete.Broadcast({}, false);
	}
	else
	{
		MultiplayerOnFindSessionsComplete.Broadcast(SearchResults, bWasSuccessful);
	}
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: OnJoinSession COMPLETED: Result=%d"), *GetName(), Result);

	if (OnlineSessionInterface)
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	JoinSessionCompleteDelegateHandle.Reset();

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}
