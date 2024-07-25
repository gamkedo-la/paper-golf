// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PaperGolfGameStateBase.generated.h"

class AGolfPlayerState;

/**
 * 
 */
UCLASS()
class PGPAWN_API APaperGolfGameStateBase : public AGameState
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHoleChanged, int32 /*Current Hole Number*/);

	FOnHoleChanged OnHoleChanged{};

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetCurrentHoleNumber() const { return CurrentHoleNumber; }

	void SetCurrentHoleNumber(int32 Hole) { CurrentHoleNumber = Hole; }

	UFUNCTION(BlueprintPure)
	AGolfPlayerState* GetActivePlayer() const { return ActivePlayer; }

	void SetActivePlayer(AGolfPlayerState* Player) { ActivePlayer = Player; }

private:
	UFUNCTION()
	void OnRep_CurrentHoleNumber();

private:

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHoleNumber)
	int32 CurrentHoleNumber{ 1 };

	UPROPERTY(Transient, Replicated)
	TObjectPtr<AGolfPlayerState> ActivePlayer{};
};
