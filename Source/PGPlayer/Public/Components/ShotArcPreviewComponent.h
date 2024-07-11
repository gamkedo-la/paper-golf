// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShotArcPreviewComponent.generated.h"

class APaperGolfPawn;
enum class EShotType : uint8;
struct FFlickParams;

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

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CalculateShotArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams);
	void DoShowShotArc();

	bool NeedsToRecalculateArc(const APaperGolfPawn& Pawn, const FFlickParams& FlickParams) const;

private:

	TArray<FVector> ArcPoints;

	FTransform LastCalculatedTransform{};
	EShotType ShotType{};
	float LocalZOffset{};

	bool bVisible{};
	bool bLastPointIsHit{};

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float MaxSimTime{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float SimFrequency{ 30.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float CollisionRadius{ 3.0f };

	UPROPERTY(Category = "Shot Arc", EditDefaultsOnly)
	float HitRadiusSize{ 100.0f };
};
