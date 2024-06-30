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

UENUM(BlueprintType)
enum class EShotType : uint8
{
	Default,
	Full,
	Close
};

/**
 * 
 */
UCLASS()
class PGPLAYER_API AGolfPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ResetRotation();

	UFUNCTION(BlueprintCallable)
	void ResetShot();

	UFUNCTION(BlueprintCallable)
	void DetermineIfCloseShot();

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Input")
	bool IsInputEnabled() const;

	void ActivateTurn();
	void Spectate(APaperGolfPawn* InPawn);

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	APaperGolfPawn* GetPaperGolfPawn();
	const APaperGolfPawn* GetPaperGolfPawn() const;

protected:
	void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ProcessFlickZInput(float FlickZInput);

	UFUNCTION(BlueprintCallable)
	void ProcessShootInput();

	UFUNCTION(BlueprintCallable)
	void AddPaperGolfPawnRelativeRotation(const FRotator& DeltaRotation);

	UFUNCTION(BlueprintCallable)
	void SetShotType(EShotType InShotType);

	UFUNCTION(BlueprintCallable)
	void ToggleShotType();

	UFUNCTION(BlueprintPure)
	EShotType GetShotType() const;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void AddStroke();

	void NextHole();

	void Init();
	void DeferredInit();

	void InitFocusableActors();

	void ResetShotAfterOutOfBounds();

	void RegisterShotFinishedTimer();
	void UnregisterShotFinishedTimer();

	void RegisterGolfSubsystemEvents();

	UFUNCTION()
	void OnOutOfBounds(APaperGolfPawn* InPaperGolfPawn);

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	UFUNCTION()
	void OnScored(APaperGolfPawn* InPaperGolfPawn);

	void HandleFallThroughFloor();
	void HandleOutOfBounds();

	bool IsReadyForNextShot() const;
	void SetupNextShot();

	void SetPaperGolfPawnAimFocus();

	void AddSpectatorPawn(APawn* PawnToSpectate);
	void SetCameraToViewPawn(APawn* InPawn);
	void SetCameraOwnedBySpectatorPawn(APawn* InPawn);

	bool HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const;

	// TODO: This will be used on server to set the authoritative position on the client
	void SetPositionTo(const FVector& Position);

	UFUNCTION(Client, Reliable)
	void ClientSetPositionTo(const FVector_NetQuantize& Position);

	void CheckForNextShot();

	UFUNCTION(Client, Reliable)
	void ClientActivateTurn();

	UFUNCTION(Client, Reliable)
	void ClientSpectate(APaperGolfPawn* InPawn);

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

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	int32 FlickZNotUpdatedMaxRetries{ 2 };

	bool bInputEnabled{ true };
	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};

	FTimerHandle SpectatorCameraDelayTimer{};

	// TODO: Do we want to allow the player to change the camera controls when spectating?
	UPROPERTY(Category = "Camera | Spectator", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float SpectatorCameraControlsDelay{ 3.0f };

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	//UPROPERTY(ReplicatedUsing = OnRep_ShotFinishedLocation)
	//FVector_NetQuantize ShotFinishedLocation{};
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

FORCEINLINE void AGolfPlayerController::SetInputEnabled(bool bEnabled)
{
	bInputEnabled = bEnabled;
}

FORCEINLINE bool AGolfPlayerController::IsInputEnabled() const
{
	return bInputEnabled;
}

#pragma endregion Inline Definitions
