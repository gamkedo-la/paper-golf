// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "Engine.h"

#include "MultiplayerSessionsLogging.h"
#include "MultiplayerSessionsSubsystem.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"

#include "Components/EditableText.h"

#include "VisualLogger/VisualLogger.h"

#include "Engine/GameInstance.h"

#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Menu)

void UMenu::MenuSetup(int32 NumberOfPublicConnections, bool bIsLanMatch, const FString& TypeOfMatch, const FString& LobbyPath)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: MenuSetup: NumberOfPublicConnections=%d; bIsLanMatch=%s; TypeOfMatch=%s; LobbyPath=%s"), 
		*GetName(), NumberOfPublicConnections, bIsLanMatch ? TEXT("TRUE") : TEXT("FALSE"), *TypeOfMatch, *LobbyPath);

	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	PathToLobby = LobbyPath;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: World is NULL"), *GetName());
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();

	if (!PlayerController)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: No PlayerController found - unable to change input mode"), *GetName());
		return;
	}

	FInputModeGameAndUI InputModeData;
	InputModeData.SetWidgetToFocus(TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);

	if (ChkLanMatch)
	{
		ChkLanMatch->SetIsChecked(bIsLanMatch);
	}

	UGameInstance* GameInstance = GetGameInstance();
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

	MultiplayerSessionsSubsystem->Configure({ bIsLanMatch });

	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessionsComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSessionComplete);
}

// This is like the constructor and can be used to bind callbacks
bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (ensureMsgf(BtnHost, TEXT("%s: BtnHost is NULL"), *GetName()))
	{
		BtnHost->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: BtnHost NULL!"), *GetName());
	}

	if (ensureMsgf(BtnJoin, TEXT("%s: BtnJoin is NULL"), *GetName()))
	{
		BtnJoin->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: BtnJoin NULL!"), *GetName());
	}

	if (ensureMsgf(ChkLanMatch, TEXT("%s: ChkLanMatch is NULL"), *GetName()))
	{
		ChkLanMatch->OnCheckStateChanged.AddDynamic(this, &ThisClass::OnLanMatchChanged);
	}
	else
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: ChkLanMatch NULL!"), *GetName());
	}

	if(!ensureMsgf(TxtLanIpAddress, TEXT("%s: TxtLanIpAddress is NULL"), *GetName()))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: TxtLanIpAddress NULL!"), *GetName());
	}

	return true;
}

void UMenu::NativeDestruct()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: NativeDestruct"), *GetName());

	MenuTeardown();
	
	Super::NativeDestruct();
}

void UMenu::OnCreateSessionComplete(bool bWasSuccessful)
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

void UMenu::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnFindSessionsComplete: SessionResultsSize=%d; bWasSuccessful=%s"),
		*GetName(), SessionResults.Num(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnFindSessionsComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
		BtnJoin->SetIsEnabled(true);
		return;
	}

	for (const auto& Result : SessionResults)
	{
		const auto& Id = Result.GetSessionIdStr();
		const auto& User = Result.Session.OwningUserName;

		FString SettingsValue;
		const bool FoundMatchType = Result.Session.SessionSettings.Get(UMultiplayerSessionsSubsystem::SessionMatchTypeName, SettingsValue);

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
				FString::Printf(TEXT("Joining Match Type: %s"), FoundMatchType ? *MatchType : TEXT("NULL"))
			);
		}

		if (MatchType == MatchType)
		{
			// Joining configured match type (e.g. FreeForAll) - get ip address

			// Register callback when session is joined
			MultiplayerSessionsSubsystem->JoinSession(Result);

			return;
		}
	}

	BtnJoin->SetIsEnabled(true);
}

void UMenu::OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
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
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Connect String: %s"), *IpAddress)
			);
		}

		// Need to client travel to this hosting player - client travel is on the player controller

		if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController(); PlayerController)
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

void UMenu::OnDestroySessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnDestroySessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void UMenu::OnStartSessionComplete(bool bWasSuccessful)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnStartSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
	BtnHost->SetIsEnabled(true);
}

void UMenu::HostButtonClicked()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: HostButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	BtnHost->SetIsEnabled(false);

	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostButtonClicked - NumPublicConnections=%d; MatchType=%s"), *GetName(), NumPublicConnections, *MatchType);

	if(IsDirectIpLanMatch())
	{
		HostDirectLanMatch();
	}
	else
	{
		HostDiscoverableMatch();
	}
}

void UMenu::JoinButtonClicked()
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

void UMenu::MenuTeardown()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: MenuTeardown"), *GetName());

	RemoveFromParent();

	auto World = GetWorld();

	if (!World)
	{
		return;
	}

	auto PlayerController = World->GetFirstPlayerController();

	if (!PlayerController)
	{
		return;
	}

	FInputModeGameOnly InputGameOnly;
	PlayerController->SetInputMode(InputGameOnly);
	PlayerController->SetShowMouseCursor(false);
}

void UMenu::HostDiscoverableMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostDiscoverableMatch"), *GetName());

	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::HostDirectLanMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: HostDirectLanMatch"), *GetName());

	MultiplayerSessionsSubsystem->CreateLocalSession(NumPublicConnections, MatchType);
}

void UMenu::OnLanMatchChanged(bool bIsChecked)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: OnLanMatchChanged: bIsChecked=%s"), *GetName(), bIsChecked ? TEXT("TRUE") : TEXT("FALSE"));

	if (!MultiplayerSessionsSubsystem)
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Warning, TEXT("%s: OnLanMatchChanged - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	MultiplayerSessionsSubsystem->Configure({ bIsChecked });
}

void UMenu::SubsystemFindMatch()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: SubsystemFindMatch"), *GetName());

	// Using a large number because if we are using the steam dev app id there could be a lot of other sessions
	MultiplayerSessionsSubsystem->FindSessions(10000);
}

void UMenu::IpConnectLanMatch()
{
	check(ChkLanMatch);
	check(TxtLanIpAddress);

	const auto PlayerController = GetOwningPlayer();
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

bool UMenu::IsDirectIpLanMatch() const
{
	if (!ChkLanMatch || !TxtLanIpAddress)
	{
		return false;
	}

	return ChkLanMatch->IsChecked() && !TxtLanIpAddress->GetText().IsEmpty();
}
