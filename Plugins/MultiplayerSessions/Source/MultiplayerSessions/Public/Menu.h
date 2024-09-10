// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "Menu.generated.h"

class UButton;
class UCheckBox;
class UEditableText;
class UComboBoxString;

class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void MenuSetup(const TMap<FString,FString>& MatchTypesToDisplayMap, const FString& LobbyPath, int32 MinPlayers = 2, int32 MaxPlayers = 4, int32 DefaultNumPlayers = 2, bool bDefaultLANMatch = true);

protected:

	virtual bool Initialize() override;

	// OnLevelRemovedFromWorld was removed in Unreal 5.1, we can use "NativeDestruct" instead to do the equivalent cleanup
	virtual void NativeDestruct() override;

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

private:

	void InitNumberOfPlayersComboBox(int32 MinPlayers, int32 MaxPlayers);
	void InitMatchTypesComboBox(const TMap<FString, FString>& MatchTypesToDisplayMap);

	// Allow invoking from blueprint for Gamepad support
	UFUNCTION(BlueprintCallable)
	void HostButtonClicked();

	UFUNCTION(BlueprintCallable)
	void JoinButtonClicked();

	UFUNCTION(BlueprintCallable)
	void OnLanMatchChanged(bool bIsChecked);

	void MenuTeardown();

	void HostDiscoverableMatch();
	void HostDirectLanMatch();

	void SubsystemFindMatch();
	void IpConnectLanMatch();

	bool IsDirectIpLanMatch() const;

	FString GetPreferredMatchType() const;
	int32 GetMaxNumberOfPlayers() const;


	const FOnlineSessionSearchResult* FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults) const;
	const FOnlineSessionSearchResult* MatchSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults, const TArray<FString> AllowedMatchTypes) const;

private:

	// Button variable in widget blueprint is bound also in C++ - name needs to match exactly
	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> BtnHost{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> BtnJoin{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UCheckBox> ChkLanMatch{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> TxtLanIpAddress{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> CboAvailableMatchTypes{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> CboMaxNumberOfPlayers{};

	// Subsystem to handle all online functionality
	UPROPERTY(Transient)
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem {};

	TMap<FString,FString> MatchDisplayNamesToMatchTypes{};
	FString PathToLobby{};
	int32 DefaultNumPlayers{};
};
