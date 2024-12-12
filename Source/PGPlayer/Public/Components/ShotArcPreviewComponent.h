// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShotArcPreviewComponent.generated.h"

class APaperGolfPawn;
enum class EShotType : uint8;
struct FFlickParams;
class UTextRenderComponent;
struct FPredictProjectilePathResult;
class UMaterialInterface;
class AShotArc;
class APlayerCameraManager;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGPLAYER_API UShotArcPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UShotArcPreviewComponent();

	UFUNCTION(BlueprintCallable)
	void ShowShotArc(const FFlickParams& FlickParams);

	UFUNCTION(BlueprintCallable)
	void HideShotArc();

	UFUNCTION(BlueprintPure)
	bool IsVisible() const { return bVisible;  }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CalculateShotArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams);

	AShotArc* SpawnShotArcActor(const APaperGolfPawn& Pawn);
	void DestroyShotArcActor();

	void DoShowShotArc();

	bool NeedsToRecalculateArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams) const;

	FVector GetPowerFractionTextLocation(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult) const;

	void UpdatePowerText(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult);
	void HidePowerText() const;
	void ShowPowerText() const;

	void RegisterPowerText(const APaperGolfPawn& Pawn);
	void UnregisterPowerText();

	const APlayerCameraManager* GetCameraManager(const APaperGolfPawn& Pawn) const;
	TOptional<FVector> GetCameraLocation(const APaperGolfPawn& Pawn) const;
	TOptional<FRotator> GetCameraRotation(const APaperGolfPawn& Pawn) const;

private:

	FTransform LastCalculatedTransform{};
	EShotType ShotType{};
	float LocalZOffset{};
	float PowerFraction{};

	bool bVisible{};

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> PowerText{};

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float MaxSimTime{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float SimFrequency{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float CollisionRadius{ 3.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float TextHorizontalOffset{ 100.0f };

	UPROPERTY(Category = "Text", EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> TextMaterial{};

	UPROPERTY(Category = "Shot Arc | Render", EditDefaultsOnly)
	TSubclassOf<AShotArc> ShotArcSpawnActorClass{};

	UPROPERTY(Transient)
	TObjectPtr<AShotArc> ShotArc{};

	/*
	* Minimum Z distance above player to display the shot power text.
	*/
	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	float ShotPowerZMinLocationOffset{ 200.0f };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	int32 ShotPowerDesiredTextPointIndex{ 4 };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	float ShotPowerTextSize{ 50.0f };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	FLinearColor ShotPowerColor{ FColor::Orange };
};
