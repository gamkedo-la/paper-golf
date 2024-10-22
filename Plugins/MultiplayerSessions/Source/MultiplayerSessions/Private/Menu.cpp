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
    
    DefaultNumPlayers = InDefaultNumPlayers;

	IMultiplayerMenu::Execute_MenuSetup(MenuHelper,
		MatchTypesToDisplayMap, Maps, LobbyPath, MinPlayers, MaxPlayers, InDefaultNumPlayers, bDefaultLANMatch, bDefaultAllowBots
	);
    
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
    
    MenuHelper->Initialize(this);

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

void UMenu::InitNumberOfPlayersComboBox(int32 MinPlayers, int32 MaxPlayers)
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

void UMenu::InitMatchTypesComboBox(const TMap<FString, FString>& MatchTypesToDisplayMap)
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

void UMenu::InitMapsComboBox(const TArray<FString>& Maps)
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

void UMenu::SetHostEnabled_Implementation(bool bEnabled)
{
    BtnHost->SetIsEnabled(bEnabled);
}

void UMenu::SetJoinEnabled_Implementation(bool bEnabled)
{
    BtnJoin->SetIsEnabled(bEnabled);
}

void UMenu::OnHostButtonClicked()
{
    // Forwarding functions to interface since must use the Execute_ variant
    IMultiplayerMenu::Execute_HostButtonClicked(this);
}

void UMenu::OnJoinButtonClicked()
{
    IMultiplayerMenu::Execute_JoinButtonClicked(this);
}

void UMenu::OnLanMatchChanged(bool bIsChecked)
{
    IMultiplayerMenu::Execute_LanMatchChanged(this, bIsChecked);
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

FString UMenu::GetPreferredMatchType_Implementation() const
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

FString UMenu::GetPreferredMap_Implementation() const
{
    return CboAvailableMaps ? CboAvailableMaps->GetSelectedOption() : FString{};
}

int32 UMenu::GetMaxNumberOfPlayers_Implementation() const
{
    return CboMaxNumberOfPlayers ? FCString::Atoi(*CboMaxNumberOfPlayers->GetSelectedOption()) : DefaultNumPlayers;
}

bool UMenu::AllowBots_Implementation() const
{
    return ChkAllowBots ? ChkAllowBots->IsChecked() : false;
}

bool UMenu::IsDirectIpLanMatch_Implementation() const
{
    if (!ChkLanMatch || !TxtLanIpAddress)
    {
        return false;
    }

    return ChkLanMatch->IsChecked() && !TxtLanIpAddress->GetText().IsEmpty();
}

FString UMenu::GetLanIpAddress_Implementation() const
{
    return TxtLanIpAddress ? TxtLanIpAddress->GetText().ToString() : FString{};
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
