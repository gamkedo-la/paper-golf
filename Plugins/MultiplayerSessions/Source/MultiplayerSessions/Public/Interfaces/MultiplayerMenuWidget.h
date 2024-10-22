// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MultiplayerMenuWidget.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Blueprintable, MinimalAPI)
class UMultiplayerMenuWidget : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MULTIPLAYERSESSIONS_API IMultiplayerMenuWidget
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    void SetHostEnabled(bool bEnabled);
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    void SetJoinEnabled(bool bEnabled);
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    FString GetPreferredMatchType() const;
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    FString GetPreferredMap() const;
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    int32 GetMaxNumberOfPlayers() const;
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    bool AllowBots() const;
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    bool IsDirectIpLanMatch() const;
    
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Menu")
    FString GetLanIpAddress() const;
};
