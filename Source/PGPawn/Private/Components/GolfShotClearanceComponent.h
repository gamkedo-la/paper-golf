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

	bool ShouldAdjustPosition() const;

	float GetActorHeight() const;

	FBox GetOwnerAABB() const;

	bool IsClearanceNeeded() const;

	bool CalculateClearanceLocation(FVector& OutNewLocation) const;

private:

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	bool bEnabled{ true };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float TargetMinDistance{ 5 * 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float RecalculationMinDistance{ 100.0f };

	UPROPERTY(Category = "Config", EditDefaultsOnly)
	float AdjustmentDistance{ 3 * 100.0f };

	mutable float ActorHeight{ -1.0f };
	FVector TargetPosition{ EForceInit::ForceInitToZero };
	FVector LastPosition{ EForceInit::ForceInitToZero };
	bool bFirstShot{ true };
};

