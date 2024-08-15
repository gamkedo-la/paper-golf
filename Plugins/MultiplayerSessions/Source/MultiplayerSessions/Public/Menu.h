// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

class UButton;
class UMultiplayerSessionsSubsystem;
class FOnlineSessionSearchResult;

namespace EOnJoinSessionCompleteResult { enum Type : int; }

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void MenuSetup(int32 NumberOfPublicConnections = 4, const FString& TypeOfMatch = TEXT("FreeForAll"), const FString& LobbyPath = TEXT("/Game/ThirdPerson/Maps/Lobby"));

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

	UFUNCTION()
	void HostButtonClicked();

	UFUNCTION()
	void JoinButtonClicked();

	void MenuTeardown();

private:

	// Button variable in widget blueprint is bound also in C++ - name needs to match exactly
	UPROPERTY(meta = (BindWidget))
	UButton* BtnHost;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnJoin;

	// Subsystem to handle all online functionality
	UPROPERTY()
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem {};

	int32 NumPublicConnections { 4 };
	FString MatchType { TEXT("FreeForAll") };

	FString PathToLobby{};
};
