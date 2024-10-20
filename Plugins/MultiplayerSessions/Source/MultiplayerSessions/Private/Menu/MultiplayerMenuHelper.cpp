// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Menu/MultiplayerMenuHelper.h"

#include "Engine.h"

#include "MultiplayerSessionsLogging.h"
#include "MultiplayerSessionsSubsystem.h"

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

bool UMultiplayerMenuHelper::Initialize(const FMultiplayerMenuWidgets& MenuWidgets)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: Initialize"), *GetName());

	BtnHost = MenuWidgets.BtnHost;
	BtnJoin = MenuWidgets.BtnJoin;
	ChkLanMatch = MenuWidgets.ChkLanMatch;
	ChkAllowBots = MenuWidgets.ChkAllowBots;
	TxtLanIpAddress = MenuWidgets.TxtLanIpAddress;
	CboAvailableMatchTypes = MenuWidgets.CboAvailableMatchTypes;
	CboMaxNumberOfPlayers = MenuWidgets.CboMaxNumberOfPlayers;
	CboAvailableMaps = MenuWidgets.CboAvailableMaps;

	// Make sure all are defined

	if (ensureMsgf(BtnHost, TEXT("%s: BtnHost is NULL"), *GetName()))
	{
		BtnHost->OnClicked.AddDynamic(this, &ThisClass::OnHostButtonClicked);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: BtnHost NULL!"), *GetName());
		return false;
	}

	if (ensureMsgf(BtnJoin, TEXT("%s: BtnJoin is NULL"), *GetName()))
	{
		BtnJoin->OnClicked.AddDynamic(this, &ThisClass::OnJoinButtonClicked);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: BtnJoin NULL!"), *GetName());
		return false;
	}

	if (ensureMsgf(ChkLanMatch, TEXT("%s: ChkLanMatch is NULL"), *GetName()))
	{
		ChkLanMatch->OnCheckStateChanged.AddDynamic(this, &ThisClass::OnLanMatchChanged);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: ChkLanMatch NULL!"), *GetName());
		return false;
	}

	if (!ensureMsgf(TxtLanIpAddress, TEXT("%s: TxtLanIpAddress is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: TxtLanIpAddress NULL!"), *GetName());
		return false;
	}

	if (!ensureMsgf(CboAvailableMatchTypes, TEXT("%s: CboAvailableMatchTypes is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: CboAvailableMatchTypes NULL!"), *GetName());
		return false;
	}

	if (!ensureMsgf(CboAvailableMaps, TEXT("%s: CboAvailableMaps is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: CboAvailableMaps NULL!"), *GetName());
		return false;
	}

	if (!ensureMsgf(CboMaxNumberOfPlayers, TEXT("%s: CboMaxNumberOfPlayers is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: CboMaxNumberOfPlayers NULL!"), *GetName());
		return false;
	}

	return true;
}

void UMultiplayerMenuHelper::MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath, int32 MinPlayers, int32 MaxPlayers, int32 InDefaultNumPlayers, bool bDefaultLANMatch, bool bDefaultAllowBots)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: MenuSetup: MinPlayers=%d; MaxPlayers=%d; DefaultNumPlayers=%d; bDefaultLANMatch=%s; DefaultAllowBots=%s; MatchTypesToDisplayMap=%d; Maps=%d; LobbyPath=%s"),
		*GetName(), MinPlayers, MaxPlayers, InDefaultNumPlayers,
		bDefaultLANMatch ? TEXT("TRUE") : TEXT("FALSE"),
		bDefaultAllowBots ? TEXT("TRUE") : TEXT("FALSE"),
		MatchTypesToDisplayMap.Num(), Maps.Num(), *LobbyPath);

	PathToLobby = LobbyPath;
	DefaultNumPlayers = InDefaultNumPlayers;

	APlayerController* PlayerController = GetLocalPlayerController();

	if (!PlayerController)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: No PlayerController found - unable to change input mode"), *GetName());
		return;
	}

	InitNumberOfPlayersComboBox(MinPlayers, MaxPlayers);
	InitMatchTypesComboBox(MatchTypesToDisplayMap);
	InitMapsComboBox(Maps);

	if (ChkLanMatch)
	{
		ChkLanMatch->SetIsChecked(bDefaultLANMatch);
	}

	if (ChkAllowBots)
	{
		ChkAllowBots->SetIsChecked(bDefaultAllowBots);
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

void UMultiplayerMenuHelper::InitNumberOfPlayersComboBox(int32 MinPlayers, int32 MaxPlayers)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: InitNumberOfPlayersComboBox: MinPlayers=%d; MaxPlayers=%d; DefaultNumPlayers=%d"), *GetName(), MinPlayers, MaxPlayers, DefaultNumPlayers);

	if (!CboMaxNumberOfPlayers)
	{
		return;
	}

	if (MinPlayers <= 0)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: InitNumberOfPlayersComboBox - MinPlayers=%d <= 0; defaulting to 1"), *GetName(), MinPlayers);
		MinPlayers = 1;
	}
	if (MaxPlayers < MinPlayers)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: InitNumberOfPlayersComboBox - MaxPlayers=%d < MinPlayers=%d; defaulting to MinPlayers"), *GetName(), MaxPlayers, MinPlayers);
		MaxPlayers = MinPlayers;
	}

	if (DefaultNumPlayers < MinPlayers || DefaultNumPlayers > MaxPlayers)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: InitNumberOfPlayersComboBox - DefaultNumPlayers=%d not in range [%d, %d]; defaulting to MinPlayers"),
			*GetName(), DefaultNumPlayers, MinPlayers, MaxPlayers);
		DefaultNumPlayers = MinPlayers;
	}

	CboMaxNumberOfPlayers->ClearOptions();
	for (int32 Index = MinPlayers; Index <= MaxPlayers; ++Index)
	{
		CboMaxNumberOfPlayers->AddOption(FString::FromInt(Index));
	}

	// Set initial selected option to DefaultNumPlayers
	CboMaxNumberOfPlayers->SetSelectedOption(FString::FromInt(DefaultNumPlayers));
}

void UMultiplayerMenuHelper::InitMatchTypesComboBox(const TMap<FString, FString>& MatchTypesToDisplayMap)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: InitMatchTypesComboBox: MatchTypesToDisplayMap=%d"), *GetName(), MatchTypesToDisplayMap.Num());

	if (!CboAvailableMatchTypes)
	{
		return;
	}

	// Map keys to the display name

	CboAvailableMatchTypes->ClearOptions();
	for (int32 Index = 0; const auto & [_, MatchDisplayName] : MatchTypesToDisplayMap)
	{
		CboAvailableMatchTypes->AddOption(MatchDisplayName);

		if (Index == 0)
		{
			CboAvailableMatchTypes->SetSelectedOption(MatchDisplayName);
		}
		++Index;
	}

	MatchDisplayNamesToMatchTypes.Reset();

	// Reverse the input map in order to get the match type for given display name
	// since UMG doesn't have concept of option keys like an HTML combobox
	for (const auto& Entry : MatchTypesToDisplayMap)
	{
		MatchDisplayNamesToMatchTypes.Add(Entry.Value, Entry.Key);
	}
}

void UMultiplayerMenuHelper::InitMapsComboBox(const TArray<FString>& Maps)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: InitMapsComboBox: Maps=%d"), *GetName(), Maps.Num());

	if (!CboAvailableMaps)
	{
		return;
	}

	CboAvailableMaps->ClearOptions();
	for (int32 Index = 0; const auto & Map : Maps)
	{
		CboAvailableMaps->AddOption(Map);

		if (Index == 0)
		{
			CboAvailableMaps->SetSelectedOption(Map);
		}
		++Index;
	}
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
		BtnHost->SetIsEnabled(true);
	}
}

void UMultiplayerMenuHelper::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnFindSessionsComplete: bWasSuccessful=%s; SessionResultsSize=%d;  PreferredMatchType=%s; MatchDisplayNamesToMatchTypes=%d"),
		*GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"), SessionResults.Num(), *GetPreferredMatchType(), MatchDisplayNamesToMatchTypes.Num());

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnFindSessionsComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
		BtnJoin->SetIsEnabled(true);
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

	BtnJoin->SetIsEnabled(true);
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
	MatchDisplayNamesToMatchTypes.GenerateValueArray(AvailableMatchTypes);

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
		BtnJoin->SetIsEnabled(true);

		return;
	}

	// Get correct ip address and client travel to the server

	auto OnlineSessionInterface = MultiplayerSessionsSubsystem->GetOnlineSessionInterface();

	if (!OnlineSessionInterface)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnJoinSessionComplete - OnlineSessionInterface is NULL"), *GetName());
		BtnJoin->SetIsEnabled(true);

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
			BtnJoin->SetIsEnabled(true);
		}
	}
	else
	{
		BtnJoin->SetIsEnabled(true);
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

void UMultiplayerMenuHelper::OnHostButtonClicked()
{
	// Forwarding functions to interface since must use the Execute_ variant
	IMultiplayerMenu::Execute_HostButtonClicked(this);
}

void UMultiplayerMenuHelper::OnJoinButtonClicked()
{
	IMultiplayerMenu::Execute_JoinButtonClicked(this);
}

void UMultiplayerMenuHelper::OnLanMatchChanged(bool bIsChecked)
{
	IMultiplayerMenu::Execute_LanMatchChanged(this, bIsChecked);
}

void UMultiplayerMenuHelper::OnDestroySessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnDestroySessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void UMultiplayerMenuHelper::OnStartSessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnStartSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
	BtnHost->SetIsEnabled(true);
}

void UMultiplayerMenuHelper::HostButtonClicked_Implementation()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: HostButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	BtnHost->SetIsEnabled(false);

	const bool bAllowBots = ChkAllowBots ? ChkAllowBots->IsChecked() : false;

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

	BtnJoin->SetIsEnabled(false);

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
	check(ChkLanMatch);
	check(TxtLanIpAddress);

	const auto PlayerController = GetLocalPlayerController();
	if (!ensure(PlayerController))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: IpConnectLanMatch - PlayerController is NULL"), *GetName());
		BtnHost->SetIsEnabled(true);
		return;
	}

	const auto& IpAddress = TxtLanIpAddress->GetText().ToString();

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: IpConnectLanMatch - %s"), *GetName(), *IpAddress);

	PlayerController->ClientTravel(IpAddress, ETravelType::TRAVEL_Absolute);
}

bool UMultiplayerMenuHelper::IsDirectIpLanMatch() const
{
	if (!ChkLanMatch || !TxtLanIpAddress)
	{
		return false;
	}

	return ChkLanMatch->IsChecked() && !TxtLanIpAddress->GetText().IsEmpty();
}

FString UMultiplayerMenuHelper::GetPreferredMatchType() const
{
	if (!CboAvailableMatchTypes)
	{
		return {};
	}

	if (const auto MatchResult = MatchDisplayNamesToMatchTypes.Find(CboAvailableMatchTypes->GetSelectedOption()); MatchResult)
	{
		return *MatchResult;
	}

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: GetPreferredMatchType - Could not find match type for display name %s"), *GetName(), *CboAvailableMatchTypes->GetSelectedOption());
	return {};
}

FString UMultiplayerMenuHelper::GetPreferredMap() const
{
	return CboAvailableMaps ? CboAvailableMaps->GetSelectedOption() : FString{};
}

int32 UMultiplayerMenuHelper::GetMaxNumberOfPlayers() const
{
	return CboMaxNumberOfPlayers ? FCString::Atoi(*CboMaxNumberOfPlayers->GetSelectedOption()) : DefaultNumPlayers;
}
