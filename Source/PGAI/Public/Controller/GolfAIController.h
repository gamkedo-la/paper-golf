// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "Interfaces/GolfController.h"

#include "PaperGolfTypes.h"

#include "Pawn/PaperGolfPawn.h"

#include "GolfAIController.generated.h"

class UGolfControllerCommonComponent;
class APaperGolfPawn;

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

	virtual void SetPawn(APawn* InPawn) override;

protected:
	virtual AController* AsController() override { return this; }
	virtual const AController* AsController() const override { return this; }

private:
	void OnScored();
	void ExecuteTurn();
	void DestroyPawn();
	void SetupNextShot(bool bSetCanFlick);
	void ResetShot();
	void DetermineShotType();
	void ResetShotAfterOutOfBounds();
	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	TOptional<FFlickParams> CalculateFlickParams() const;
	FFlickParams CalculateDefaultFlickParams() const;

private:
	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfControllerCommonComponent> GolfControllerCommonComponent{};

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float MinFlickReactionTime{ 2.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float MaxFlickReactionTime{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float BounceOverhitCorrectionFactor{ 0.05f };

	// TODO: Use curve table
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AccuracyDeviation{ 0.2f };

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	FTimerHandle TurnTimerHandle{};

	EShotType ShotType{ EShotType::Default };
	bool bScored{};
	bool bTurnActivated{};
	bool bOutOfBounds{};
};

#pragma region Inline Definitions

FORCEINLINE EShotType AGolfAIController::GetShotType() const
{
	return ShotType;
}

FORCEINLINE bool AGolfAIController::HasScored() const
{
	return bScored;
}

FORCEINLINE bool AGolfAIController::IsActivePlayer() const
{
	return bTurnActivated;
}

#pragma endregion Inline Definitions
