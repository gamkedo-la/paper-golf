// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PaperGolfTypes.h"

#include "Subsystems/GolfEvents.h"

#include "GolfController.generated.h"

class APaperGolfPawn;
class AGolfPlayerState;
class UGolfControllerCommonComponent;
class UGolfEventsSubsystem;

// This class does not need to be modified.
UINTERFACE(BlueprintType, NotBlueprintable, MinimalAPI)
class UGolfController : public UInterface
{
	GENERATED_BODY()
};

/**
 * Base interface implemented by both player and AI golf controllers.
 * Used to invoke golf-specific logic from the game mode and supporting components generically without needing to know if the controller is a player or AI.
 */
class PGPAWN_API IGolfController
{
	GENERATED_BODY()

public:
	virtual void MarkScored() = 0;
	virtual bool HasScored() const = 0;
	virtual bool IsActivePlayer() const = 0;
	virtual bool IsReadyForNextShot() const = 0;

	/*
	* Player is active player and already flicked but the paper golf pawn is still in motion.
	*/
	virtual bool IsActiveShotInProgress() const = 0;


	/*
	* Receive the player start that is used for the starting location for this player.  In the sequence of start actions, the following order occurs.
	* 
	* 1) ReceivePlayerStart
	* 2) StartHole
	* 3) ActivateTurn
	* 
	*/
	virtual void ReceivePlayerStart(AActor* PlayerStart) = 0;
	/*
	* Called when it is this player's turn for the first time on a hole.
	* Called right before ActivateTurn on the server.
	*/
	virtual void StartHole(EHoleStartType HoleStartType) = 0;
	virtual void ActivateTurn() = 0;
	virtual void Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* PlayerState) = 0;

	virtual bool HandleHazard(EHazardType HazardType) = 0;

	virtual bool HasPaperGolfPawn() const = 0;

	virtual APaperGolfPawn* GetPaperGolfPawn() = 0;
	const APaperGolfPawn* GetPaperGolfPawn() const;

	virtual EShotType GetShotType() const = 0;

	FString ToString() const;

	AGolfPlayerState* GetGolfPlayerState();
	const AGolfPlayerState* GetGolfPlayerState() const;

	void AddStroke();

	virtual AController* AsController() = 0;
	const AController* AsController() const;

	bool IsSpectatorOnly() const;
	void SetSpectatorOnly();

	virtual void ResetShot() = 0;

	AActor* GetCurrentGolfHole() const;

protected:
	void DoBeginPlay(const TFunction<void(UGolfEventsSubsystem&)>& ClippedThroughWorldRegistrator);

	virtual UGolfControllerCommonComponent* GetGolfControllerCommonComponent() = 0;
	UGolfControllerCommonComponent* GetGolfControllerCommonComponent() const; 

	virtual void DoAdditionalOnShotFinished() = 0;
	virtual void DoAdditionalFallThroughFloor() = 0;

	/*
	* Invoke from derived class in DoBeginPlay registered UFUNCTION() that handles the initial event
	*/
	void DoOnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);


private:
	void RegisterGolfSubsystemEvents(const TFunction<void(UGolfEventsSubsystem&)>& ClippedThroughWorldRegistrator);

	void OnShotFinished();

	void HandleFallThroughFloor();
};

#pragma region Inline Definitions

FORCEINLINE FString IGolfController::ToString() const
{
	return AsController()->GetName();
}

FORCEINLINE const APaperGolfPawn* IGolfController::GetPaperGolfPawn() const
{
	return const_cast<IGolfController*>(this)->GetPaperGolfPawn();
}

FORCEINLINE const AController* IGolfController::AsController() const
{
	return const_cast<IGolfController*>(this)->AsController();
}

FORCEINLINE UGolfControllerCommonComponent* IGolfController::GetGolfControllerCommonComponent() const
{
	return const_cast<IGolfController*>(this)->GetGolfControllerCommonComponent();
}

#pragma endregion Inline Definitions
