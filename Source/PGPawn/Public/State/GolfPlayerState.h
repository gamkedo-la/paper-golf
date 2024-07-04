// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GolfPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API AGolfPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void AddShot() { ++Shots;  }

	UFUNCTION(BlueprintPure)
	int32 GetShots() const { return Shots; }

	UFUNCTION(BlueprintCallable)
	void SetReadyForShot(bool bReady) { bReadyForShot = bReady; }

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const { return bReadyForShot; }

private:
	UPROPERTY(Replicated)
	int32 Shots{};

	UPROPERTY(Replicated)
	bool bReadyForShot{};
};
