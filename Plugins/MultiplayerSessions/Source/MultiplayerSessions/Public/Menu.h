// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "Menu.generated.h"

class UButton;
class UCheckBox;
class UEditableText;

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
	void MenuSetup(int32 NumberOfPublicConnections = 4, bool bIsLanMatch = true, const FString& TypeOfMatch = TEXT("FreeForAll"), const FString& LobbyPath = TEXT("/Game/ThirdPerson/Maps/Lobby"));

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

	// Subsystem to handle all online functionality
	UPROPERTY(Transient)
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem {};

	int32 NumPublicConnections { 4 };
	FString MatchType { TEXT("FreeForAll") };

	FString PathToLobby{};
};
