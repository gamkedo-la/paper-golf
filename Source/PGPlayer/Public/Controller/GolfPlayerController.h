// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfPlayerController.generated.h"

class APaperGolfPawn;
class AGolfHole;

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
class PGPLAYER_API AGolfPlayerController : public APlayerController, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:
	AGolfPlayerController();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

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

	UFUNCTION(BlueprintPure)
	bool HasScored() const;

	void MarkScored();

	UFUNCTION(BlueprintPure)
	bool IsActivePlayer() const;

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

	// Called on both clients and server via a rep notify
	virtual void SetPawn(APawn* InPawn) override;

private:

	void InitDebugDraw();

	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void AddStroke();

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

	void HandleFallThroughFloor();
	void HandleOutOfBounds();

	bool IsReadyForNextShot() const;
	void SetupNextShot(bool bSetCanFlick);

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

	void DoActivateTurn();

	UFUNCTION(Server, Reliable)
	void ServerProcessShootInput();

	UFUNCTION()
	void OnRep_Scored();

	void OnScored();

	UFUNCTION(BlueprintPure)
	bool CanFlick() const;

	UFUNCTION(BlueprintPure)
	AGolfHole* GetGolfHole() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<FShotHistory> ShotHistory{};
	

private:
#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator RotationMax{ 75.0, 180.0, 90.0 };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator TotalRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RotationRate{ 100.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float RestCheckTickRate{ 0.5f };

	float FlickZ{ };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FLinearColor FlickReticuleColor{ };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Focus")
	TSubclassOf<UInterface> FocusableActorClass{};

	UPROPERTY(Transient)
	TArray<AActor*> FocusableActors{};

	UPROPERTY(Transient)
	TObjectPtr<AGolfHole> GolfHole{};

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	float FallThroughFloorCorrectionTestZ{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	int32 FlickZNotUpdatedMaxRetries{ 2 };

	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};

	FTimerHandle SpectatorCameraDelayTimer{};

	// TODO: Do we want to allow the player to change the camera controls when spectating?
	UPROPERTY(Category = "Camera | Spectator", EditDefaultsOnly)
	float SpectatorCameraControlsDelay{ 3.0f };

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	bool bCanFlick{ };
	bool bTurnActivated{};
	bool bInputEnabled{ true };

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Scored)
	bool bScored{};

	bool bOutOfBounds{};
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

FORCEINLINE bool AGolfPlayerController::HasScored() const
{
	return bScored;
}

FORCEINLINE bool AGolfPlayerController::CanFlick() const
{
	return bCanFlick;
}

FORCEINLINE AGolfHole* AGolfPlayerController::GetGolfHole() const
{
	return nullptr;
}

FORCEINLINE bool AGolfPlayerController::IsActivePlayer() const
{
	return bTurnActivated;
}

#pragma endregion Inline Definitions
