// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GolfShotClearanceComponent.generated.h"


UCLASS()
class UGolfShotClearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGolfShotClearanceComponent();

	bool AdjustPositionForClearance();

	void SetTargetPosition(const FVector& InTargetPosition) { TargetPosition = InTargetPosition; }

	bool IsEnabled() const { return bEnabled; }

protected:
	virtual void BeginPlay() override;

private:

	struct FHitResultData
	{
		FVector Location;
		FVector Normal;
	};

	struct FClearanceLocationResult
	{
		FVector CurrentPosition;
		FVector UpVector;
		FVector PushbackDirection;
		FVector NewPosition;
	};

	struct FClearanceTraceParams
	{
		float TraceHalfHeight;
		FVector NewPositionTraceStart;
		FVector NewPositionTraceEnd;
	};

private:

	bool ShouldAdjustPosition() const;
	bool PositionAdjustmentNotAllowed() const;

	float GetActorHeight() const;

	FBox GetOwnerAABB() const;

	bool IsClearanceNeeded(FHitResultData& OutHitResultData) const;
	bool CalculateClearanceLocation(const FHitResultData& OutHitResultData, FVector& OutNewLocation) const;
	bool CalculateClearanceLocation(const FVector& HitLocation, const FVector& PushbackDirection, FVector& OutNewLocation) const;

	TOptional<FClearanceLocationResult> CalculateClearanceLocationResult(const FVector& HitLocation, const FVector& RawPushbackDirection) const;
	bool CheckForNewLocationCollisions(const FClearanceLocationResult& ClearanceLocationResult, FClearanceTraceParams& OutClearanceTraceParams) const;
	bool CheckNewLocationPlayable(const FClearanceLocationResult& ClearanceLocationResult, const FClearanceTraceParams& ClearanceTraceParams) const;
private:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bEnabled{ true };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float TargetMinDistance{ 5 * 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float RecalculationMinDistance{ 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float AdjustmentDistance{ 3 * 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float GroundDeltaAllowance{ 2 * 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float HitNormalAlignmentAngle{ 45.0f };

	float HitNormalAlignmentAngleCos{};

	mutable float ActorHeight{ -1.0f };
	float LastPositionUpdateWorldTime{ -1.0f };
	FVector TargetPosition{ EForceInit::ForceInitToZero };
	FVector LastPosition{ EForceInit::ForceInitToZero };
	bool bFirstShot{ true };
};

