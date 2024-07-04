// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "PaperGolfGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API APaperGolfGameStateBase : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	int32 GetCurrentHole() const { return CurrentHole; }

	void SetCurrentHole(int32 Hole) { CurrentHole = Hole; }

private:

	// TODO: Note that game state does not persist across server travel
	// If we want to take a hole per map approach (for mix and matching courses),
	// we would need to use the server's game instance to cache the current course state and then restore it in InitGameState in the GameMode
	// The preferred approach at this time though is for all holes in a course to be in the same map and then there would be no travel across holes in a course
	// which would provide a more seamless experience
	// We would use "Reset" on the player controller and player state to reset the data for the next hole
	// In either case the CurrentHole would be accessed from here from other actors in the game
	// The player start associated with a particular hole number on the map could be identified via an actor tag
	UPROPERTY(Replicated)
	int32 CurrentHole{ 1 };
	
};
