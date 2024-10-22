// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/MultiplayerMenu.h"

#include "MultiplayerMenuHelper.generated.h"

class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;
class APlayerController;
class IMultiplayerMenuWidget;

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MULTIPLAYERSESSIONS_API UMultiplayerMenuHelper : public UObject, public IMultiplayerMenu
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable)
    void Initialize(const TScriptInterface<IMultiplayerMenuWidget>& InMenuWidget);

protected:

    virtual void MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath,
        int32 MinPlayers = 2, int32 MaxPlayers = 4, int32 DefaultNumPlayers = 2, bool bDefaultLANMatch = true, bool bDefaultAllowBots = false) override;
    
	virtual void HostButtonClicked_Implementation() override;

	virtual void JoinButtonClicked_Implementation() override;

	virtual void LanMatchChanged_Implementation(bool bIsChecked) override;

	//
	// Callbacks for the custom delegates on the MultiplayerSessionSubsystem
	// 
	UFUNCTION()
	virtual void OnCreateSessionComplete(bool bWasSuccessful);

	virtual void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	virtual void OnJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

	UFUNCTION()
	virtual void OnDestroySessionComplete(bool bWasSuccessful);

	UFUNCTION()
	virtual void OnStartSessionComplete(bool bWasSuccessful);

	APlayerController* GetLocalPlayerController() const;

private:

	void HostDiscoverableMatch();
	void HostDirectLanMatch();

	void SubsystemFindMatch();
	void IpConnectLanMatch();

	bool IsDirectIpLanMatch() const;

	const FOnlineSessionSearchResult* FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults) const;
	const FOnlineSessionSearchResult* MatchSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults, const TArray<FString> AllowedMatchTypes) const;
    
    void SetHostEnabled(bool bEnabled);
    void SetJoinEnabled(bool bEnabled);
    FString GetPreferredMatchType() const;
    FString GetPreferredMap() const;
    int32 GetMaxNumberOfPlayers() const;

protected:

	// Subsystem to handle all online functionality
	UPROPERTY(Transient, Category = "Multiplayer", BlueprintReadOnly)
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem{};

    UPROPERTY(Transient)
    TObjectPtr<UObject> MultiplayerWidget;

    TArray<FString> AllAvailableMatchTypes{};
	FString PathToLobby{};
    int32 DefaultNumPlayers{};
};
