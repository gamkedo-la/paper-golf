// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Interfaces/GolfController.h"

#include "GolfAIController.generated.h"

class UGolfControllerCommonComponent;

/**
 * 
 */
UCLASS()
class PGAI_API AGolfAIController : public AAIController, public IGolfController
{
	GENERATED_BODY()
	
public:

	AGolfAIController();

	// Inherited via IGolfController
	using IGolfController::GetPaperGolfPawn;
	virtual APaperGolfPawn* GetPaperGolfPawn() override;

	virtual void MarkScored() override;

	virtual bool HasScored() const override;

	virtual bool IsActivePlayer() const override;

	virtual bool IsReadyForNextShot() const override;

	virtual void ActivateTurn() override;

	virtual void Spectate(APaperGolfPawn* InPawn) override;

	virtual bool HandleOutOfBounds() override;

	virtual EShotType GetShotType() const override;

protected:
	virtual AController* AsController() override { return this; }
	virtual const AController* AsController() const override { return this; }

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfControllerCommonComponent> GolfControllerCommonComponent{};
};
