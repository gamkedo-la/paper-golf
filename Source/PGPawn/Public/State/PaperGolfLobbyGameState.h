// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PaperGolfLobbyGameState.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API APaperGolfLobbyGameState : public AGameState
{
	GENERATED_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStatesUpdated, APaperGolfLobbyGameState*, LobbyGameState, int32, NumPlayers);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Initialize(const FString& InGameModeName, int32 InMinPlayers, int32 InMaxPlayers);

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	// Checks if game state was initialized.  Only valid on the server.
	bool IsInitialized() const { return bInitialized; }

	UFUNCTION(BlueprintPure)
	FString GetGameModeName() const { return GameModeName; }

	UFUNCTION(BlueprintPure)
	int32 GetMinPlayers() const { return MinPlayers; }

	UFUNCTION(BlueprintPure)
	int32 GetMaxPlayers() const { return MaxPlayers; }

	UFUNCTION(BlueprintPure)
	bool IsGameEligibleToStart() const;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	FOnPlayerStatesUpdated OnPlayerStatesUpdated{};

private:
	UPROPERTY(Transient, Replicated)
	FString GameModeName{};

	UPROPERTY(Transient, Replicated)
	int32 MinPlayers{};

	UPROPERTY(Transient, Replicated)
	int32 MaxPlayers{};

	bool bInitialized{};
};
