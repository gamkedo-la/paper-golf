// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PaperGolfTypes.h"

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

// TODO: Cannot have BlueprintPure UFUNCTIONs on interfaces
// If have BlueprintCallable functions that aren't events, then must mark NotBlueprintable
/**
 * 
 */
class PGPAWN_API IGolfController
{
	GENERATED_BODY()

public:
	virtual void MarkScored() = 0;
	virtual bool HasScored() const = 0;
	virtual bool IsActivePlayer() const = 0;
	virtual bool IsReadyForNextShot() const = 0;
	virtual void ActivateTurn() = 0;
	virtual void Spectate(APaperGolfPawn* InPawn) = 0;

	virtual bool HandleOutOfBounds() = 0;

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
