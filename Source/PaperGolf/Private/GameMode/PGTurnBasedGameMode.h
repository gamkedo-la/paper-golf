// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/PaperGolfGameModeBase.h"
#include "PGTurnBasedGameMode.generated.h"

class UGolfTurnBasedDirectorComponent;

/**
 * 
 */
UCLASS()
class APGTurnBasedGameMode : public APaperGolfGameModeBase
{
	GENERATED_BODY()

public:
	APGTurnBasedGameMode();


protected:

	virtual void StartHole(int32 HoleNumber) override;

	virtual void OnPlayerJoined(AController* NewPlayer) override;
	virtual void OnPlayerLeft(AController* Exiting) override;
	virtual void OnPlayerReplaced(AController* LeavingPlayer, AController* NewPlayer) override;

	virtual void OnGameStart() override;

	virtual bool DelayStartWithTimer() const override;

	virtual int32 GetNumberOfActivePlayers() const override;

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfTurnBasedDirectorComponent> TurnBasedDirectorComponent{};

};
