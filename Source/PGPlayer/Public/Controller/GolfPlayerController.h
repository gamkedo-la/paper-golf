// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.h"

#include "GameFramework/PlayerController.h"

#include "Interfaces/GolfController.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfPlayerController.generated.h"

class APaperGolfPawn;
class AGolfHole;
class UShotArcPreviewComponent;

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
class PGPLAYER_API AGolfPlayerController : public APlayerController, public IGolfController, public IVisualLoggerDebugSnapshotInterface
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

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Input")
	bool IsInputEnabled() const;

	virtual void ActivateTurn() override;
	virtual void Spectate(APaperGolfPawn* InPawn) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	virtual APaperGolfPawn* GetPaperGolfPawn() override;
	virtual const APaperGolfPawn* GetPaperGolfPawn() const override;;

	UFUNCTION(BlueprintPure)
	virtual bool HasScored() const override;

	virtual void MarkScored() override;

	UFUNCTION(BlueprintPure)
	virtual bool IsActivePlayer() const override;

	virtual bool HandleOutOfBounds() override;

	UFUNCTION(BlueprintPure)
	virtual EShotType GetShotType() const override;

	virtual FString ToString() const override { return GetName(); }

	virtual AGolfPlayerState* GetGolfPlayerState() override;
	virtual const AGolfPlayerState* GetGolfPlayerState() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ResetForCamera();

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

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// Called on both clients and server via a rep notify
	virtual void SetPawn(APawn* InPawn) override;

private:

	void DetermineShotType();

	void InitDebugDraw();
	void CleanupDebugDraw();
	void DestroyPawn();

	void AddToShotHistory(APaperGolfPawn* PaperGolfPawn);

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void Init();
	void DeferredInit();

	void InitFocusableActors();

	void ResetShotAfterOutOfBounds();

	void RegisterShotFinishedTimer();
	void UnregisterShotFinishedTimer();

	void RegisterGolfSubsystemEvents();

	UFUNCTION(Client, Reliable)
	void ClientHandleOutOfBounds();

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	void HandleFallThroughFloor();

	bool IsReadyForNextShot() const;
	void SetupNextShot(bool bSetCanFlick);

	void SetPaperGolfPawnAimFocus();

	void AddSpectatorPawn(APawn* PawnToSpectate);
	void SetCameraToViewPawn(APawn* InPawn);
	void SetCameraOwnedBySpectatorPawn(APawn* InPawn);

	bool HasLOSToFocus(const FVector& Position, const AActor* FocusActor) const;

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	UFUNCTION(Client, Reliable)
	void ClientSetTransformTo(const FVector_NetQuantize& Position, const FRotator& Rotation);

	UFUNCTION(Client, Reliable)
	void ClientResetShotAfterOutOfBounds(const FVector_NetQuantize& Position);

	void CheckForNextShot();

	UFUNCTION(Client, Reliable)
	void ClientActivateTurn();

	UFUNCTION(Client, Reliable)
	void ClientSpectate(APaperGolfPawn* InPawn);

	void DoActivateTurn();

	UFUNCTION(Server, Reliable)
	void ServerProcessShootInput(const FRotator& InTotalRotation);

	UFUNCTION()
	void OnRep_Scored();

	void OnScored();

	UFUNCTION(BlueprintPure)
	bool CanFlick() const;

	UFUNCTION(BlueprintPure)
	AGolfHole* GetGolfHole() const;

	bool ShouldEnableInputForActivateTurn() const;

	UFUNCTION(BlueprintPure)
	float GetFlickZ() const;

private:
	TArray<FShotHistory> ShotHistory{};
	
#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShotArcPreviewComponent> ShotArcPreviewComponent{};

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

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float CloseShotThreshold{ 1000.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Type")
	float MediumShotThreshold{ 3000.0f };

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

FORCEINLINE EShotType AGolfPlayerController::GetShotType() const
{
	return ShotType;
}

FORCEINLINE float AGolfPlayerController::GetFlickZ() const
{
	return FlickZ;
}

#pragma endregion Inline Definitions
