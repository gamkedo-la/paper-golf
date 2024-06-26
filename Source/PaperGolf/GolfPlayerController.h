// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GolfPlayerController.generated.h"

class APaperGolfPawn;

USTRUCT(BlueprintType)
struct FShotHistory
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FVector Position{ EForceInit::ForceInitToZero };

};

bool operator== (const FShotHistory& First, const FShotHistory& Second);


/**
 * 
 */
UCLASS()
class PAPERGOLF_API AGolfPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ResetRotation();

	UFUNCTION(BlueprintCallable)
	void ResetShot();

	UFUNCTION(BlueprintCallable)
	void SetPaperGolfPawnAimFocus();

	UFUNCTION(BlueprintCallable)
	void DetermineIfCloseShot();

	UFUNCTION(BlueprintCallable)
	void OnScored();

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const;

protected:
	void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void InitFocusableActors();

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ProcessFlickZInput(float FlickZInput);

	UFUNCTION(BlueprintCallable)
	void ProcessShootInput();

	UFUNCTION(BlueprintCallable)
	void AddPaperGolfPawnRelativeRotation(const FRotator& DeltaRotation);

	UFUNCTION(BlueprintCallable)
	void CheckAndSetupNextShot();

	UFUNCTION(BlueprintCallable)
	void HandleFallThroughFloor();

	// TODO: May want to move this to game mode
	UFUNCTION(BlueprintCallable)
	void HandleOutOfBounds();

private:
	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void AddStroke();

	void NextHole();

	void ResetShotAfterOutOfBounds();

private:
	APaperGolfPawn* GetPaperGolfPawn();
	const APaperGolfPawn* GetPaperGolfPawn() const;

	bool HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const;

	void SetPositionTo(const FVector& Position);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<FShotHistory> ShotHistory{};

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	bool bCanFlick{ true };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	FRotator RotationMax{ 75.0, 180.0, 90.0 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	FRotator TotalRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float RotationRate{ 100.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float RestCheckTickRate{ 0.5f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float FlickZ{ };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	FLinearColor FlickReticuleColor{ };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	bool bScored{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float NextHoleDelay{ 3.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	bool bOutOfBounds{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<UInterface> FocusableActorClass{};

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<AActor> GolfHoleClass{};

	UPROPERTY(EditDefaultsOnly, Transient, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	TArray<AActor*> FocusableActors{};

	UPROPERTY(EditDefaultsOnly, Transient, BlueprintReadWrite, Category = "Controller", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> DefaultFocus{};

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	float FallThroughFloorCorrectionTestZ{ 1000.0f };
};

#pragma region Inline Definitions

FORCEINLINE bool operator== (const FShotHistory& First, const FShotHistory& Second)
{
	return First.Position.Equals(Second.Position, 1.0);
}

FORCEINLINE bool AGolfPlayerController::IsReadyForShot() const
{
	return bCanFlick;
}

#pragma endregion Inline Definitions
