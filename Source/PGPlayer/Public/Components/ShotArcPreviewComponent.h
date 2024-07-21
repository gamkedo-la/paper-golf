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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CalculateShotArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams);
	void DoShowShotArc();

	bool NeedsToRecalculateArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams) const;

	FVector GetPowerFractionTextLocation(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult) const;

	void UpdatePowerText(const APaperGolfPawn& Pawn, const FPredictProjectilePathResult& PredictResult);
	void HidePowerText();
	void ShowPowerText();

	void RegisterPowerText(const APaperGolfPawn& Pawn);
	void UnregisterPowerText();

private:

	TArray<FVector> ArcPoints;

	FTransform LastCalculatedTransform{};
	EShotType ShotType{};
	float LocalZOffset{};
	float PowerFraction{};

	bool bVisible{};
	bool bLastPointIsHit{};

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> PowerText{};

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float MaxSimTime{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float SimFrequency{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float CollisionRadius{ 3.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float HitRadiusSize{ 100.0f };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	float ShotPowerZLocationOffset{ 200.0f };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	float ShotPowerTextSize{ 50.0f };

	UPROPERTY(Category = "Shot Arc | Power", EditDefaultsOnly)
	FLinearColor ShotPowerColor{ FColor::Orange };
};
