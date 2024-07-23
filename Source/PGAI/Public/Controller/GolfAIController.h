// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

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

	virtual void ResetForNextHole() override;

protected:
	virtual AController* AsController() override { return this; }

	virtual void BeginPlay() override;

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

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	// Inherited via IGolfController
	virtual UGolfControllerCommonComponent* GetGolfControllerCommonComponent() override;
	virtual void DoAdditionalOnShotFinished() override;
	virtual void DoAdditionalFallThroughFloor() override;

	float GenerateAccuracy() const;
	float GeneratePowerFraction(float InPowerFraction) const;

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
	float BounceOverhitCorrectionFactor{ 0.075f };

	// TODO: Use curve table
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AccuracyDeviation{ 0.3f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float PowerDeviation{ 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinShotPower{ 0.1f };

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	FTimerHandle TurnTimerHandle{};

	EShotType ShotType{ EShotType::Default };

	bool bCanFlick{ };
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

FORCEINLINE UGolfControllerCommonComponent* AGolfAIController::GetGolfControllerCommonComponent()
{
	return GolfControllerCommonComponent;
}

#pragma endregion Inline Definitions
