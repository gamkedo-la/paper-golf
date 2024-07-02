// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PaperGolfGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
// TODO: Change to AGameMode and use custom match states
// See https://www.udemy.com/course/unreal-engine-5-cpp-multiplayer-shooter/learn/lecture/31670022
class PAPERGOLF_API APaperGolfGameModeBase : public AGameMode
{
	GENERATED_BODY()


public:

	APaperGolfGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;

	virtual void GenericPlayerInitialization(AController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	virtual void StartMatch() override;
	virtual void EndMatch() override;
	virtual void RestartGame() override;
	virtual void AbortMatch() override;

	/** called before startmatch */
	virtual void HandleMatchIsWaitingToStart() override;

	/** Called when match starts */
	virtual void HandleMatchHasStarted() override;

	void SetDesiredNumberOfPlayers(int32 InDesiredNumberOfPlayers);

	// Begin Player start selection functions
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	// End Player start selection functions

	int32 GetTotalNumberOfPlayers() const;

protected:
	virtual void OnMatchStateSet() override;
	virtual void BeginPlay() override;

	/*
	* Called by derived classes to notify when the hole is about to start
	*/
	void NotifyHoleAboutToStart();

	// TODO: See MustSpectate_Implementation in AGameMode for an idea of how to start players as spectators - possibly default implementation is fine as we can
	// Set spectating on player state as it expects

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;

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
private:
	bool SetDesiredNumberOfPlayersFromPIESettings();

private:
	UPROPERTY(Category = "Config", EditDefaultsOnly)
	int32 DefaultDesiredNumberOfPlayers{ 1 };

	int32 DesiredNumberOfPlayers{};

	bool bAllowPlayerSpawn{};
};

#pragma region Inline Definitions

FORCEINLINE void APaperGolfGameModeBase::SetDesiredNumberOfPlayers(int32 InDesiredNumberOfPlayers)
{
	this->DesiredNumberOfPlayers = InDesiredNumberOfPlayers;
}

FORCEINLINE int32 APaperGolfGameModeBase::GetTotalNumberOfPlayers() const
{
	return NumPlayers + NumBots;
}

#pragma endregion Inline Definitions
