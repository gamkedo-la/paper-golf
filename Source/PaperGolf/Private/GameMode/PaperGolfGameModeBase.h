// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PaperGolfGameModeBase.generated.h"

class AGolfAIController;
class UHoleTransitionComponent;
class IGolfController;
class UPlayerConfig;
class APaperGolfPawn;

namespace PG
{
	class FPlayerStateConfigurator;
}

/**
 * Base game mode for all paper golf game modes.
 */
UCLASS(Abstract)
class PAPERGOLF_API APaperGolfGameModeBase : public AGameMode
{
	GENERATED_BODY()


public:

	APaperGolfGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;

	virtual void Logout(AController* Exiting) override final;

	virtual void StartMatch() override;
	virtual void EndMatch() override;
	virtual void RestartGame() override;
	virtual void AbortMatch() override;

	/** called before startmatch */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Called when match starts */
	virtual void HandleMatchHasStarted() override;

	// Begin Player start selection functions
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	// End Player start selection functions

	int32 GetTotalNumberOfPlayers() const;

	virtual void Reset() override;

	int32 GetMinimumNumberOfPlayers() const;

protected:

	virtual void StartHole(int32 HoleNumber) PURE_VIRTUAL(APaperGolfGameModeBase::StartHole, );

	virtual void OnMatchStateSet() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*
	* Called by derived classes to notify when the hole is about to start
	*/
	void NotifyHoleAboutToStart();

	// TODO: See MustSpectate_Implementation in AGameMode for an idea of how to start players as spectators - possibly default implementation is fine as we can
	// Set spectating on player state as it expects

	virtual bool DelayStartWithTimer() const;

	// Called for both seamless and non-seamless travel cases for generic initialization
	virtual void GenericPlayerInitialization(AController* NewPlayer) override final;

	// Override these to kick players that join when the server is full
	virtual void HandleSeamlessTravelPlayer(AController*& C) override final;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override final;

	virtual bool ReadyToStartMatch_Implementation() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// Begin Player start selection functions
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	/** select best spawn point for player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	// Looks like FindPlayerStart_Implementation is never overriden in any of the examples as it relies on the above hook functions to customize the behavior
	// The default implementation looks fine for our purposes as we will be sure to choose the right player start
	// End Player start selection functions

	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	virtual void OnGameStart() PURE_VIRTUAL(APaperGolfGameModeBase::OnGameStart, );


	/*
	* Called immediately after the hole is complete.  The hole transition component will invoke start hole after its configured hole delay time.
	* This is an opportunity to do additional processing before the next hole starts.  An example would be to signal the player controllers to display the results
	* from the last hole since the player state has been updated, though clients will need to wait for the appropriate rep notifies to fire with the updated fields.
	*/
	virtual void DoAdditionalHoleComplete() {}

	/*
	* Called when the course is complete. Returns the time to wait before restarting the game or returning to the main menu.
	*/
	virtual float OnCourseCompleteAction() { return CourseCompleteActionDelayTimeSeconds; }

	virtual void OnPlayerJoined(AController* NewPlayer);
	virtual void OnPlayerLeft(AController* LeavingPlayer) {}

	/* Called when a player is leaving and another is immediately replacing.
	* Use case is when a player leaves the game early and is replaced with a bot or a player late joins and replaces a bot.
	* By default this just calls OnPlayerLeft followed by OnPlayerJoined. Override completely to customize.
	*/
	virtual void OnPlayerReplaced(AController* LeavingPlayer, AController* NewPlayer);

	virtual int32 GetNumberOfActivePlayers() const { return NumPlayers;  }

	/*
	* Chooses a bot to evict when a player joins but the game is full.
	* By default chooses the bot with the best score.
	*/
	virtual AController* ChooseBotToEvict() const;

	/*
	* Invoked after course completes to decide what to do next. By default it restarts the course if configured so; otherwise, returns to the main menu.
	*/
	virtual void CourseCompletionNextAction();

private:

	void SetDesiredNumberOfPlayers(int32 InDesiredNumberOfPlayers);
	void SetDesiredNumberOfBotPlayers(int32 InDesiredNumberOfBotPlayers);

	void HandlePlayerLeaving(AController* LeavingPlayer);
	void HandlePlayerJoining(AController* NewPlayer);
	void ReplacePlayer(AController* LeavingPlayer, AController* NewPlayer);

	bool SetDesiredNumberOfPlayersFromPIESettings();

	void CreateBots();

	void DestroyBot(AController* BotToEvict);
	/*
	* Adds another bot player to the match up to max number of players.
	*/
	AGolfAIController* AddBot();
	AGolfAIController* CreateBot(int32 BotNumber);
	AGolfAIController* ReplaceLeavingPlayerWithBot(AController* Player);
	void InitBot(AGolfAIController& AIController, int32 BotNumber);
	int32 CreateBotPlayerId() const;

	void ConfigureJoinedPlayerState(AController& Player);
	void SetHumanPlayerName(AController& PlayerController, APlayerState& PlayerState);
	bool TrySetHumanPlayerNameFromMultiplayerSessions(AController& PlayerController, APlayerState& PlayerState);

	void StartGame();
	void StartGameWithDelay();

	UFUNCTION()
	void OnStartHole(int32 HoleNumber);

	UFUNCTION()
	void OnHoleComplete();

	UFUNCTION()
	void OnCourseComplete();

	void DoCourseComplete();

	void InitPlayerStateDefaults();

	void InitNumberOfPlayers(const FString& Options);

	void SetNumberOfPlayersFromOptions(const FString& Options);
	void InitFromConsoleVars();

	void CheckDefaultOptions();

	void DetermineAllowBots(const FString& Options);

	bool ValidateJoinMatch(AController* Controller);

	bool MatchIsJoinable() const;
	bool MatchIsActive() const;
	bool MatchShouldBeAbandoned() const;
	bool IsWorldBeingDestroyed() const;

	int32 GetMaximumNumberOfPlayers() const;
	int32 GetTotalPlayersToStartMatch() const;

	void KickPlayer(AController* Player, const TCHAR* Reason);

	virtual bool CanCourseBeStarted() const;

	UClass* GetControllerPawnClass(AController* InController) const;

protected:
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	int32 StartHoleNumber{ 1 };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bAllowBots{ true };

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UHoleTransitionComponent> HoleTransitionComponent{};

	// TODO: Consider making config UPROPERTY versions of these to be set from ini files or overriden from command line on server travel. Maybe can even just add "config" to existing?
	// Looks like this works but "actor defaults have higher priority then config"
	// See https://forums.unrealengine.com/t/changing-editable-uproperty-variable-to-be-a-config-variable-is-ignored/413067/6
	/*
	*  UPROPERTY(config)
	*  int32 ConfigNumberOfBotPlayers{};
	*/

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	int32 DefaultDesiredNumberOfPlayers{ 1 };

	/*
	* Sets the minimum number of players for the mode.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	int32 MinNumberOfPlayers{ 1 };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TSubclassOf<AGolfAIController> AIControllerClass{};

	/*
	* Set minimum number of bot players. Mainly for testing.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	int32 DefaultMinDesiredNumberOfBotPlayers{ 0 };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float MatchStartDelayTime{ 1.0f };

	/*
	* Indicates if the game should cycle back to hole 1 when the course is complete.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bRestartGameOnCourseComplete{ true };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float CourseCompleteActionDelayTimeSeconds{ 5.0f };

	int32 DesiredNumberOfPlayers{};
	int32 DesiredNumberOfBotPlayers{};
	int32 MaxPlayers{};

	int32 HumanPlayerDefaultNameIndex{};

	bool bAllowPlayerSpawn{};
	bool bWasReset{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TObjectPtr<UPlayerConfig> PlayerConfigData{};

	/*
	* Customize which pawn classes are used to spawn each controller type.
	*/
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	TMap<TSubclassOf<AController>, TSubclassOf<APaperGolfPawn>> ControllerPawnClassMap{};

	TUniquePtr<PG::FPlayerStateConfigurator> PlayerStateConfigurator{};
};

#pragma region Inline Definitions

FORCEINLINE void APaperGolfGameModeBase::SetDesiredNumberOfPlayers(int32 InDesiredNumberOfPlayers)
{
	DesiredNumberOfPlayers = InDesiredNumberOfPlayers;
}

FORCEINLINE void APaperGolfGameModeBase::SetDesiredNumberOfBotPlayers(int32 InDesiredNumberOfBotPlayers)
{
	DesiredNumberOfBotPlayers = InDesiredNumberOfBotPlayers;
}

FORCEINLINE int32 APaperGolfGameModeBase::GetTotalNumberOfPlayers() const
{
	return NumPlayers + NumBots;
}

FORCEINLINE int32 APaperGolfGameModeBase::GetMinimumNumberOfPlayers() const
{
	return MinNumberOfPlayers;
}

FORCEINLINE int32 APaperGolfGameModeBase::GetMaximumNumberOfPlayers() const
{
	return MaxPlayers;
}

FORCEINLINE int32 APaperGolfGameModeBase::GetTotalPlayersToStartMatch() const
{
	return DesiredNumberOfPlayers + DesiredNumberOfBotPlayers;
}

#pragma endregion Inline Definitions
