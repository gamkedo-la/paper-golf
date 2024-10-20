// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/MultiplayerMenu.h"

#include "MultiplayerMenuHelper.generated.h"

class UButton;
class UCheckBox;
class UEditableText;
class UComboBoxString;

class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;
class APlayerController;

USTRUCT(BlueprintType)
struct MULTIPLAYERSESSIONS_API FMultiplayerMenuWidgets
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UButton> BtnHost{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UButton> BtnJoin{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCheckBox> ChkLanMatch{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UCheckBox> ChkAllowBots{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UEditableText> TxtLanIpAddress{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UComboBoxString> CboAvailableMatchTypes{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UComboBoxString> CboMaxNumberOfPlayers{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UComboBoxString> CboAvailableMaps{};
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MULTIPLAYERSESSIONS_API UMultiplayerMenuHelper : public UObject, public IMultiplayerMenu
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "Menu")
	virtual bool Initialize(const FMultiplayerMenuWidgets& MenuWidgets);

	virtual void MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath,
		int32 MinPlayers = 2, int32 MaxPlayers = 4, int32 DefaultNumPlayers = 2, bool bDefaultLANMatch = true, bool bDefaultAllowBots = false) override;

protected:

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

	UFUNCTION()
	void OnHostButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	UFUNCTION()
	void OnLanMatchChanged(bool bIsChecked);

	void InitNumberOfPlayersComboBox(int32 MinPlayers, int32 MaxPlayers);
	void InitMatchTypesComboBox(const TMap<FString, FString>& MatchTypesToDisplayMap);
	void InitMapsComboBox(const TArray<FString>& Maps);

	void HostDiscoverableMatch();
	void HostDirectLanMatch();

	void SubsystemFindMatch();
	void IpConnectLanMatch();

	bool IsDirectIpLanMatch() const;

	FString GetPreferredMatchType() const;
	FString GetPreferredMap() const;
	int32 GetMaxNumberOfPlayers() const;

	const FOnlineSessionSearchResult* FindBestSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults) const;
	const FOnlineSessionSearchResult* MatchSessionResult(const TArray<FOnlineSessionSearchResult>& SessionResults, const TArray<FString> AllowedMatchTypes) const;

protected:

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UButton> BtnHost{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UButton> BtnJoin{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UCheckBox> ChkLanMatch{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UCheckBox> ChkAllowBots{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UEditableText> TxtLanIpAddress{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UComboBoxString> CboAvailableMatchTypes{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UComboBoxString> CboMaxNumberOfPlayers{};

	UPROPERTY(Transient, Category = "Menu", BlueprintReadOnly)
	TObjectPtr<UComboBoxString> CboAvailableMaps{};

	// Subsystem to handle all online functionality
	UPROPERTY(Transient, Category = "Multiplayer", BlueprintReadOnly)
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem{};

	TMap<FString, FString> MatchDisplayNamesToMatchTypes{};
	TArray<FString> AvailableMaps{};

	FString PathToLobby{};
	int32 DefaultNumPlayers{};
};
