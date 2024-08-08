// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "MultiplayerSessionsLogging.h"

#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h" 

#include "VisualLogger/VisualLogger.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MultiplayerSessionsSubsystem)

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

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (!OnlineSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnlineSubsystem is NULL"), *GetName());
		return;
	}

	OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Deinitialize"), *GetName());

	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

		OnlineSessionInterface.Reset();
	}

	Super::Deinitialize();
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, const FString& MatchType)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: CreateSession: NumPublicConnections=%d; MatchType=%s"), *GetName(), NumPublicConnections, *MatchType);

	if (!OnlineSessionInterface.IsValid())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: CreateSession - OnlineSessionInterface is not valid: NumPublicConnections=%d; MatchType=%s"), 
			*GetName(), NumPublicConnections, *MatchType);

		MultiplayerOnCreateSessionComplete.Broadcast(false);

		return;
	}

	const auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Destroying existing NamedSession: NAME_GameSession"), *GetName());

		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		bCreateSessionOnDestroy = true;

		DestroySession();
		
		return;
	}

	DesiredNumPublicConnections = NumPublicConnections;
	DesiredMatchType = MatchType;

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
	LastSessionSettings->Set(SessionMatchTypeName, MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	LastSessionSettings->BuildUniqueId = 1; // So that we can join other sessions

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
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: CreateSession FAILED: NumPublicConnections=%d; MatchType=%s"), *GetName(), NumPublicConnections, *MatchType);

		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();

		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: CreateSession InProgress: NumPublicConnections=%d; MatchType=%s"), *GetName(), NumPublicConnections, *MatchType);

		// Don't call MultiplayerOnCreateSessionComplete.Broadcast(true) yet as we need to wait for the OnCreateSessionComplete function to trigger
	}
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
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

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
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: DestroySession - OnlineSessionInterface is not valid"), *GetName());

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
		CreateSession(LastNumPublicConnections, LastMatchType);
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
	if (bWasSuccessful)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: OnFindSessions COMPLETED: SearchResultsFound=%d"),
			*GetName(), LastSessionSearch ? LastSessionSearch->SearchResults.Num() : -1);
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

	check(LastSessionSearch);

	if (LastSessionSearch->SearchResults.IsEmpty())
	{
		MultiplayerOnFindSessionsComplete.Broadcast({}, false);
	}
	else
	{
		MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
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

bool UMultiplayerSessionsSubsystem::IsLanMatch() const
{
	auto OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: OnlineSubsystem is NULL"), *GetName());
		return false;
	}

	// LAN match if the subsystem is NULL (We are not using steam or other online subsystem)
	bool bIsLANMatch = OnlineSubsystem->GetSubsystemName() == "NULL";

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: IsLanMatch=%s"), *GetName(), (bIsLANMatch ? TEXT("TRUE") : TEXT("FALSE")));

	return bIsLANMatch;
}
