// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);


/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UMultiplayerSessionsSubsystem();

	struct FSessionsConfiguration
	{
		bool bIsLanMatch{};
	};

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void Configure(const FSessionsConfiguration& InConfiguration);

	void SetBuildId(int32 InBuildId);
	void CreateSession(int32 NumPublicConnections, const FString& MatchType);
	void CreateLocalSession(int32 NumPublicConnections, const FString& MatchType);

	void FindSessions(int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& OnlineSessionSearchResult);

	void StartSession();
	void DestroySession();

	IOnlineSessionPtr GetOnlineSessionInterface() const;

	/// 
	/// Our own custom delegates for the menu class to bind callbacks to
	///
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

	inline static const FName SessionMatchTypeName = "MatchType";

	int32 GetDesiredNumPublicConnections() const;
	FString GetDesiredMatchType() const;

	UFUNCTION(BlueprintPure)
	bool IsLanMatch() const;

protected:

	// Internal callbacks we'll add for the delegates we'll add to the OnlineSessionInterface delegate list
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	void DestroyOnlineSubsystem();
	void SetSubsystemEnabled(const FName& SubsystemName, bool bIsEnabled);
private:
	IOnlineSessionPtr OnlineSessionInterface{};
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings{};

	// Need a delegate for each session action: Create, Find, Join, Start, Destroy
	// to add to the IOnlineSessionInterface delegate list
	// We'll bind our internal session callbacks to these delegates

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate{};
	FDelegateHandle CreateSessionCompleteDelegateHandle{};

	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate{};
	FDelegateHandle FindSessionsCompleteDelegateHandle{};
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch{};

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate{};
	FDelegateHandle JoinSessionCompleteDelegateHandle{};

	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate{};
	FDelegateHandle StartSessionCompleteDelegateHandle{};

	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate{};
	FDelegateHandle DestroySessionCompleteDelegateHandle{};

	bool bCreateSessionOnDestroy{};
	int32 LastNumPublicConnections{};
	FString LastMatchType{};

	int32 DesiredNumPublicConnections{};
	FString DesiredMatchType{};
	int32 BuildId{};
	FSessionsConfiguration SessionsConfiguration{};
	FName LastSubsystemName{};
};

FORCEINLINE IOnlineSessionPtr UMultiplayerSessionsSubsystem::GetOnlineSessionInterface() const
{
	return OnlineSessionInterface;
}

FORCEINLINE int32 UMultiplayerSessionsSubsystem::GetDesiredNumPublicConnections() const
{
	return DesiredNumPublicConnections;
}

FORCEINLINE FString UMultiplayerSessionsSubsystem::GetDesiredMatchType() const
{
	return DesiredMatchType;
}

FORCEINLINE void UMultiplayerSessionsSubsystem::SetBuildId(int32 InBuildId)
{
	BuildId = InBuildId;
}

FORCEINLINE bool UMultiplayerSessionsSubsystem::IsLanMatch() const
{
	return SessionsConfiguration.bIsLanMatch;
}