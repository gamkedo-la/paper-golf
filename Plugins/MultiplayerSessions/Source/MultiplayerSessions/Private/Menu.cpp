// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "Engine.h"

#include "MultiplayerSessionsLogging.h"
#include "MultiplayerSessionsSubsystem.h"

#include "Components/Button.h"

#include "MultiplayerSessionsLogging.h"

#include "Engine/GameInstance.h"

#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, const FString& TypeOfMatch, const FString& LobbyPath)
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: MenuSetup: NumberOfPublicConnections=%d; TypeOfMatch=%s; LobbyPath=%s"), 
		*GetName(), NumberOfPublicConnections, *TypeOfMatch, *LobbyPath);

	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	PathToLobby = LobbyPath;

	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: World is NULL"), *GetName());
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();

	if (!PlayerController)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: No PlayerController found - unable to change input mode"), *GetName());
		return;
	}

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: GameInstance is NULL"), *GetName());
		return;
	}

	MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

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

	if (BtnHost)
	{
		BtnHost->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	else
	{
		UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: BtnHost NULL!"), *GetName());
	}

	if (BtnJoin)
	{
		BtnJoin->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
	else
	{
		UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: BtnJoin NULL!"), *GetName());
	}

	return true;
}

void UMenu::NativeDestruct()
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: NativeDestruct"), *GetName());

	MenuTeardown();
	
	Super::NativeDestruct();
}

void UMenu::OnCreateSessionComplete(bool bWasSuccessful)
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: OnCreateSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (GEngine)
	{
		if (bWasSuccessful)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Green,
				FString(TEXT("OnCreateSessionComplete Success!"))
			);
		}
		else
		{
			BtnHost->SetIsEnabled(true);

			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("OnCreateSessionComplete FAILED!"))
			);
		}
	}

	if (bWasSuccessful)
	{
		// Travel to the lobby level after confirmation that session creation was successful
		if (auto World = GetWorld(); World)
		{
			World->ServerTravel(PathToLobby + "?listen");
		}
		else
		{
			UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: HostButtonClicked - World is NULL"), *GetName());
		}
	}
}

void UMenu::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: OnFindSessionsComplete: SessionResultsSize=%d; bWasSuccessful=%s"),
		*GetName(), SessionResults.Num(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: OnFindSessionsComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
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
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: OnJoinSessionComplete: Result=%d"), *GetName(), Result);

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: OnJoinSessionComplete - MultiplayerSessionsSubsystem is NULL"), *GetName());
		BtnJoin->SetIsEnabled(true);

		return;
	}

	// Get correct ip address and client travel to the server

	auto OnlineSessionInterface = MultiplayerSessionsSubsystem->GetOnlineSessionInterface();

	if (!OnlineSessionInterface)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: OnJoinSessionComplete - OnlineSessionInterface is NULL"), *GetName());
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
			UE_LOG(LogMultiplayerSessions, Display, TEXT("%s: OnJoinSessionComplete - Traveling to IpAddress = %s"), *GetName(), *IpAddress);
			PlayerController->ClientTravel(IpAddress, ETravelType::TRAVEL_Absolute);
		}
		else
		{
			UE_LOG(LogMultiplayerSessions, Error, TEXT("%s: OnJoinSessionComplete - No Local player controller to travel to ip address = %s!"), *GetName(), *IpAddress);
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
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: OnDestroySessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void UMenu::OnStartSessionComplete(bool bWasSuccessful)
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: OnStartSessionComplete: bWasSuccessful=%s"), *GetName(), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
	BtnHost->SetIsEnabled(true);
}

void UMenu::HostButtonClicked()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: HostButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	BtnHost->SetIsEnabled(false);

	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: HostButtonClicked - NumPublicConnections=%d; MatchType=%s"), *GetName(), NumPublicConnections, *MatchType);

	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::JoinButtonClicked()
{
	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogMultiplayerSessions, Warning, TEXT("%s: JoinButtonClicked - MultiplayerSessionsSubsystem is NULL"), *GetName());
		return;
	}

	BtnJoin->SetIsEnabled(false);

	// Using a large number because if we are using the steam dev app id there could be a lot of other sessions
	MultiplayerSessionsSubsystem->FindSessions(10000);
}

void UMenu::MenuTeardown()
{
	UE_LOG(LogMultiplayerSessions, Log, TEXT("%s: MenuTeardown"), *GetName());

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
