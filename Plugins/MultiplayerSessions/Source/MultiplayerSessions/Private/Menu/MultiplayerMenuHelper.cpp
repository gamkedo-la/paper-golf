// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Menu/MultiplayerMenuHelper.h"

#include "Engine.h"

#include "MultiplayerSessionsLogging.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Interfaces/MultiplayerMenuWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"

#include "Components/ComboBoxString.h"

#include "Components/EditableText.h"

#include "VisualLogger/VisualLogger.h"

#include "Engine/GameInstance.h"

#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MultiplayerMenuHelper)


void UMultiplayerMenuHelper::Initialize(const TScriptInterface<IMultiplayerMenuWidget>& InMenuWidget)
{
    UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Initialize"), *GetName());
    
    if(!ensure(InMenuWidget))
    {
        UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: Initialize - InMenuWidget is NULL"), *GetName());
        return;
    }
    
    MultiplayerWidget = InMenuWidget.GetObject();
}


void UMultiplayerMenuHelper::MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath, int32 MinPlayers, int32 MaxPlayers, int32 InDefaultNumPlayers, bool bDefaultLANMatch, bool bDefaultAllowBots)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: MenuSetup: MinPlayers=%d; MaxPlayers=%d; DefaultNumPlayers=%d; bDefaultLANMatch=%s; DefaultAllowBots=%s; MatchTypesToDisplayMap=%d; Maps=%d; LobbyPath=%s"),
		*GetName(), MinPlayers, MaxPlayers, InDefaultNumPlayers,
		bDefaultLANMatch ? TEXT("TRUE") : TEXT("FALSE"),
		bDefaultAllowBots ? TEXT("TRUE") : TEXT("FALSE"),
		MatchTypesToDisplayMap.Num(), Maps.Num(), *LobbyPath);

	PathToLobby = LobbyPath;
    MatchTypesToDisplayMap.GenerateKeyArray(AllAvailableMatchTypes);
    DefaultNumPlayers = InDefaultNumPlayers;

	APlayerController* PlayerController = GetLocalPlayerController();

	if (!PlayerController)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: No PlayerController found - unable to change input mode"), *GetName());
		return;
	}
	
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: GameInstance is NULL"), *GetName());
		return;
	}

	MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	MultiplayerSessionsSubsystem->Configure({ bDefaultLANMatch, true });

	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessionsComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSessionComplete);
}

void UMultiplayerMenuHelper::OnCreateSessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnCreateSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (bWasSuccessful)
	{
		// Travel to the lobby level after confirmation that session creation was successful
		if (auto World = GetWorld(); World)
		{
			World->ServerTravel(PathToLobby + "?listen");
		}
		else
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: HostButtonClicked - World is NULL"), *GetName());
		}
	}
	else
	{
        SetHostEnabled(true);
	}
}

void UMultiplayerMenuHelper::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnFindSessionsComplete: bWasSuccessful=%s; SessionResultsSize=%d;  PreferredMatchType=%s; AllAvailableMatchTypes=%d"),
		*GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"), SessionResults.Num(), *GetPreferredMatchType(), AllAvailableMatchTypes.Num());

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnFindSessionsComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
        SetJoinEnabled(true);
        
		return;
	}

	const auto* MatchedSession = FindBestSessionResult(SessionResults);;
	if (MatchedSession)
	{
		MultiplayerSessionsSubsystem->JoinSession(*MatchedSession);
		return;
	}

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: OnFindSessionsComplete - Could not find a viable session to join out of %d results"),
		*GetName(), SessionResults.Num());

    SetJoinEnabled(true);
}

const FOnlineSessionSearchResult* UMultiplayerMenuHelper::FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults) const
{
    const auto& PreferredMatchType = GetPreferredMatchType();

	// First try and join preferred match type
	TArray<FString> AvailableMatchTypes;

	if (!PreferredMatchType.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: FindBestSessionResult - Trying to join preferred match type: %s"), *GetName(), *PreferredMatchType);
		AvailableMatchTypes.Add(PreferredMatchType);
		const auto* MatchedSession = MatchSessionResult(SessionResults, AvailableMatchTypes);

		if (MatchedSession)
		{
			return MatchedSession;
		}
	}

	AvailableMatchTypes.Reset();
    AvailableMatchTypes.Append(AllAvailableMatchTypes);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: FindBestSessionResult - Trying to join %s"), *GetName(), !AvailableMatchTypes.IsEmpty()
		? *FString::Printf(TEXT("any of %d supported match types"), AvailableMatchTypes.Num()) : TEXT("Any Match Type"));

	const auto* MatchedSession = MatchSessionResult(SessionResults, AvailableMatchTypes);
	if (MatchedSession)
	{
		return MatchedSession;
	}

	return nullptr;
}

const FOnlineSessionSearchResult* UMultiplayerMenuHelper::MatchSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults, const TArray<FString> AllowedMatchTypes) const
{
	for ([[maybe_unused]] int32 Index = 0; const auto & Result : SessionResults)
	{
		const auto& Id = Result.GetSessionIdStr();
		const auto& User = Result.Session.OwningUserName;

		FString SettingsValue;
		const bool FoundMatchType = Result.Session.SessionSettings.Get(UMultiplayerSessionsSubsystem::SessionMatchTypeName, SettingsValue);

		UE_VLOG_UELOG(this, LogMultiplayerSessions, Verbose, TEXT("%s: MatchSessionResult - CHECKING - Result %d/%d - Id=%s; User=%s; MatchType=%s; NumOpenConnections=%d"),
			*GetName(), Index + 1, SessionResults.Num(), *Id, *User, FoundMatchType ? *SettingsValue : TEXT("NULL"), Result.Session.NumOpenPublicConnections);

		if (FoundMatchType && (AllowedMatchTypes.IsEmpty() || AllowedMatchTypes.Contains(SettingsValue)))
		{
			// Joining configured match type (e.g. FreeForAll) - get ip address
#if !UE_BUILD_SHIPPING
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("Id: %s, User: %s"), *Id, *User)
				);

				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("Joining Match Type: %s"), *SettingsValue)
				);
			}
#endif

			UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: MatchSessionResult - MATCHED - Result %d/%d - Id=%s; User=%s; MatchType=%s; NumOpenConnections=%d"),
				*GetName(), Index + 1, SessionResults.Num(), *Id, *User, FoundMatchType ? *SettingsValue : TEXT("NULL"), Result.Session.NumOpenPublicConnections);

			return &Result;
		}
		++Index;
	}

	return nullptr;
}

void UMultiplayerMenuHelper::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnJoinSessionComplete: Result=%d"), *GetName(), Result);

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnJoinSessionComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
        SetJoinEnabled(true);
        
		return;
	}

	// Get correct ip address and client travel to the server

	auto OnlineSessionInterface = MultiplayerSessionsSubsystem->GetOnlineSessionInterface();

	if (!OnlineSessionInterface)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnJoinSessionComplete - OnlineSessionInterface is NULL"), *GetName());
        SetJoinEnabled(true);
        
		return;
	}

	// Get the ip address

	FString IpAddress;

	if (Result == EOnJoinSessionCompleteResult::Success && OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, IpAddress))
	{
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Connect String: %s"), *IpAddress)
			);
		}
#endif

		// Need to client travel to this hosting player - client travel is on the player controller

		if (APlayerController* PlayerController = GetLocalPlayerController(); PlayerController)
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Display, TEXT("%s: OnJoinSessionComplete - Traveling to IpAddress = %s"), *GetName(), *IpAddress);
			PlayerController->ClientTravel(IpAddress, ETravelType::TRAVEL_Absolute);
		}
		else
		{
			UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: OnJoinSessionComplete - No Local player controller to travel to ip address = %s!"), *GetName(), *IpAddress);
            SetJoinEnabled(true);
        }
	}
	else
	{
        SetJoinEnabled(true);
    }
}

APlayerController* UMultiplayerMenuHelper::GetLocalPlayerController() const
{

	UGameInstance* const GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetFirstLocalPlayerController();
}



void UMultiplayerMenuHelper::OnDestroySessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnDestroySessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void UMultiplayerMenuHelper::OnStartSessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnStartSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
	
    SetHostEnabled(true);
}

void UMultiplayerMenuHelper::HostButtonClicked_Implementation()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: HostButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

    SetHostEnabled(false);
    
	const bool bAllowBots = IMultiplayerMenuWidget::Execute_AllowBots(MultiplayerWidget);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostButtonClicked - MaxNumPlayers=%d; MatchType=%s; AllowBots=%s"),
		*GetName(), GetMaxNumberOfPlayers(), *GetPreferredMatchType(),
		bAllowBots ? TEXT("TRUE") : TEXT("FALSE"));

	MultiplayerSessionsSubsystem->SetAllowBots(bAllowBots);

	if (IsDirectIpLanMatch())
	{
		HostDirectLanMatch();
	}
	else
	{
		HostDiscoverableMatch();
	}
}

void UMultiplayerMenuHelper::JoinButtonClicked_Implementation()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: JoinButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

    SetJoinEnabled(false);
    
	// If lan mode and we specified an IP - just client travel directly there
	if (IsDirectIpLanMatch())
	{
		IpConnectLanMatch();
	}
	else
	{
		SubsystemFindMatch();
	}
}

void UMultiplayerMenuHelper::HostDiscoverableMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostDiscoverableMatch"), *GetName());

	MultiplayerSessionsSubsystem->CreateSession(GetMaxNumberOfPlayers(), GetPreferredMatchType(), GetPreferredMap());
}

void UMultiplayerMenuHelper::HostDirectLanMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostDirectLanMatch"), *GetName());

	MultiplayerSessionsSubsystem->CreateLocalSession(GetMaxNumberOfPlayers(), GetPreferredMatchType(), GetPreferredMap());
}

void UMultiplayerMenuHelper::LanMatchChanged_Implementation(bool bIsChecked)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: LanMatchChanged: bIsChecked=%s"), *GetName(), bIsChecked ? TEXT("TRUE") : TEXT("FALSE"));

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: LanMatchChanged - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	MultiplayerSessionsSubsystem->Configure({ bIsChecked });
}

void UMultiplayerMenuHelper::SubsystemFindMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: SubsystemFindMatch"), *GetName());

	// Using a large number because if we are using the steam dev app id there could be a lot of other sessions
	MultiplayerSessionsSubsystem->FindSessions(10000);
}

void UMultiplayerMenuHelper::IpConnectLanMatch()
{
	const auto PlayerController = GetLocalPlayerController();
	if (!ensure(PlayerController))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: IpConnectLanMatch - PlayerController is NULL"), *GetName());
        
        SetHostEnabled(true);
        
        return;
	}

    const auto& IpAddress = IMultiplayerMenuWidget::Execute_GetLanIpAddress(MultiplayerWidget);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: IpConnectLanMatch - %s"), *GetName(), *IpAddress);

	PlayerController->ClientTravel(IpAddress, ETravelType::TRAVEL_Absolute);
}

bool UMultiplayerMenuHelper::IsDirectIpLanMatch() const
{
    return MultiplayerWidget ? IMultiplayerMenuWidget::Execute_IsDirectIpLanMatch(MultiplayerWidget) : false;
}

void UMultiplayerMenuHelper::SetHostEnabled(bool bEnabled)
{
    if(MultiplayerWidget)
    {
        IMultiplayerMenuWidget::Execute_SetHostEnabled(MultiplayerWidget, bEnabled);
    }
}

void UMultiplayerMenuHelper::SetJoinEnabled(bool bEnabled)
{
    if(MultiplayerWidget)
    {
        IMultiplayerMenuWidget::Execute_SetJoinEnabled(MultiplayerWidget, bEnabled);
    }
}

FString UMultiplayerMenuHelper::GetPreferredMatchType() const
{
    return MultiplayerWidget ? IMultiplayerMenuWidget::Execute_GetPreferredMatchType(MultiplayerWidget) : FString{};
}

FString UMultiplayerMenuHelper::GetPreferredMap() const
{
    return MultiplayerWidget ? IMultiplayerMenuWidget::Execute_GetPreferredMap(MultiplayerWidget) : FString{};
}

int32 UMultiplayerMenuHelper::GetMaxNumberOfPlayers() const
{
    return MultiplayerWidget ? IMultiplayerMenuWidget::Execute_GetMaxNumberOfPlayers(MultiplayerWidget) : DefaultNumPlayers;
}
