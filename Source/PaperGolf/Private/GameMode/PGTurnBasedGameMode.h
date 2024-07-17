// Copyright Game Salutes. All Rights Reserved.

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

	virtual void Logout(AController* Exiting) override;

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;

	virtual void OnMatchStateSet() override;

	virtual void OnBotSpawnedIntoGame(AGolfAIController& AIController, int32 BotNumber) override;

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfTurnBasedDirectorComponent> TurnBasedDirectorComponent{};
};
