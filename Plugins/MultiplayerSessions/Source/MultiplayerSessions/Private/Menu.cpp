// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"

#include "Menu/MultiplayerMenuHelper.h"

#include "MultiplayerSessionsLogging.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"

#include "Components/ComboBoxString.h"

#include "Components/EditableText.h"

#include "VisualLogger/VisualLogger.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(Menu)

void UMenu::MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath, int32 MinPlayers, int32 MaxPlayers, int32 InDefaultNumPlayers, bool bDefaultLANMatch, bool bDefaultAllowBots)
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: MenuSetup: MinPlayers=%d; MaxPlayers=%d; DefaultNumPlayers=%d; bDefaultLANMatch=%s; DefaultAllowBots=%s; MatchTypesToDisplayMap=%d; Maps=%d; LobbyPath=%s"), 
		*GetName(), MinPlayers, MaxPlayers, InDefaultNumPlayers,
		bDefaultLANMatch ? TEXT("TRUE") : TEXT("FALSE"), 
		bDefaultAllowBots ? TEXT("TRUE") : TEXT("FALSE"),
		MatchTypesToDisplayMap.Num(), Maps.Num(), *LobbyPath);

	if (!MenuHelper)
	{
		return;
	}

	IMultiplayerMenu::Execute_MenuSetup(MenuHelper,
		MatchTypesToDisplayMap, Maps, LobbyPath, MinPlayers, MaxPlayers, InDefaultNumPlayers, bDefaultLANMatch, bDefaultAllowBots
	);

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
}

// This is like the constructor and can be used to bind callbacks
bool UMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	MenuHelper = NewObject<UMultiplayerMenuHelper>(this);

	if (!ensure(MenuHelper))
	{
		UE_VLOG_UELOG(this, LogMultiplayerSessions, Error, TEXT("%s: Initialize - MenuHelper could not be constructed"),
			*GetName());
		return false;
	}

	return MenuHelper->Initialize(FMultiplayerMenuWidgets
	{ 
		.BtnHost = BtnHost,
		.BtnJoin = BtnJoin,
		.ChkLanMatch = ChkLanMatch,
		.ChkAllowBots = ChkAllowBots,
		.TxtLanIpAddress = TxtLanIpAddress,
		.CboAvailableMatchTypes = CboAvailableMatchTypes,
		.CboMaxNumberOfPlayers = CboMaxNumberOfPlayers,
		.CboAvailableMaps = CboAvailableMaps 
	});
}

void UMenu::NativeDestruct()
{
	UE_VLOG_UELOG(this, LogMultiplayerSessions, Log, TEXT("%s: NativeDestruct"), *GetName());

	MenuTeardown();
	
	Super::NativeDestruct();
}

void UMenu::HostButtonClicked_Implementation()
{
	if (!MenuHelper)
	{
		return;
	}

	IMultiplayerMenu::Execute_HostButtonClicked(MenuHelper);
}

void UMenu::JoinButtonClicked_Implementation()
{
	if (!MenuHelper)
	{
		return;
	}

	IMultiplayerMenu::Execute_JoinButtonClicked(MenuHelper);
}

void UMenu::LanMatchChanged_Implementation(bool bIsChecked)
{
	if (!MenuHelper)
	{
		return;
	}

	IMultiplayerMenu::Execute_LanMatchChanged(MenuHelper, bIsChecked);
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
