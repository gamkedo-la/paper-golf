// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MultiplayerMenu.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMultiplayerMenu : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MULTIPLAYERSESSIONS_API IMultiplayerMenu
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
	void MenuSetup(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath,
		int32 MinPlayers = 2, int32 MaxPlayers = 4, int32 DefaultNumPlayers = 2, bool bDefaultLANMatch = true, bool bDefaultAllowBots = false);

	// Allow invoking from blueprint for Gamepad support
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
	void HostButtonClicked();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
	void JoinButtonClicked();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
	void LanMatchChanged(bool bIsChecked);
};
