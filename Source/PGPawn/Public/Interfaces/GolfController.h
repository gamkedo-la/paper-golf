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
	virtual const AController* AsController() const = 0;

	bool IsSpectatorOnly() const;
	void SetSpectatorOnly();
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

#pragma endregion Inline Definitions
