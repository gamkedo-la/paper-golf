// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "PaperGolfTypes.h"

#include "GolfController.generated.h"

class APaperGolfPawn;
class AGolfPlayerState;

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

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void MarkScored() = 0;
	virtual bool HasScored() const = 0;
	virtual bool IsActivePlayer() const = 0;
	virtual bool IsReadyForShot() const = 0;
	virtual void ActivateTurn() = 0;
	virtual void Spectate(APaperGolfPawn* InPawn) = 0;

	virtual void HandleOutOfBounds() = 0;

	virtual APaperGolfPawn* GetPaperGolfPawn() = 0;
	virtual const APaperGolfPawn* GetPaperGolfPawn() const = 0;

	virtual EShotType GetShotType() const = 0;

	virtual FString ToString() const = 0;

	virtual AGolfPlayerState* GetGolfPlayerState() = 0;
	virtual const AGolfPlayerState* GetGolfPlayerState() const = 0;
};
