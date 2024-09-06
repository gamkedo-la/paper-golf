// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.h"

#include "Controller/BasePlayerController.h"

#include "Interfaces/GolfController.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfPlayerController.generated.h"

class AGolfPlayerState;
class APaperGolfPawn;
class AGolfHole;
class UShotArcPreviewComponent;
class UGolfControllerCommonComponent;
class APaperGolfGameStateBase;
class IPawnCameraLook;

/**
 * 
 */
UCLASS()
class PGPLAYER_API AGolfPlayerController : public ABasePlayerController, public IGolfController
{
	GENERATED_BODY()

public:
	AGolfPlayerController();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable)
	virtual void ResetShot() override;

	UFUNCTION(BlueprintPure)
	bool IsReadyForShot() const;

	virtual bool IsReadyForNextShot() const override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Input")
	bool IsInputEnabled() const;

	using IGolfController::GetPaperGolfPawn;
	virtual APaperGolfPawn* GetPaperGolfPawn() override;

	virtual void ReceivePlayerStart(AActor* PlayerStart) override;
	virtual void StartHole() override;
	virtual void ActivateTurn() override;
	virtual void Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	virtual bool HasScored() const override;

	virtual void MarkScored() override;

	// TODO: pull up variable to IGolfController as protected
	UFUNCTION(BlueprintPure)
	virtual bool IsActivePlayer() const override;

	virtual bool HandleOutOfBounds() override;

	UFUNCTION(BlueprintPure)
	virtual EShotType GetShotType() const override;

	virtual void Reset() override;

	virtual void AddPitchInput(float Val) override;

	virtual void AddYawInput(float Val) override;

	virtual bool IsLookInputIgnored() const override { return false; }

	UFUNCTION(BlueprintPure)
	bool IsSpectatingShotSetup() const;

	// TODO: Can we remove UFUNCTION on some of these
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void ResetForCamera();

	UFUNCTION(BlueprintCallable)
	void ResetCameraRotation();

	UFUNCTION(BlueprintCallable)
	void AddCameraZoomDelta(float ZoomDelta);

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
	virtual void PawnPendingDestroy(APawn* InPawn) override;

	// Called on both clients and server via a rep notify
	virtual void SetPawn(APawn* InPawn) override;

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintActivateTurn();

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintResetShot();

	virtual void ClientReset_Implementation() override;

	void TriggerHoleFlybyAndPlayerCameraIntroduction();

	UFUNCTION()
	void OnHoleComplete();

	virtual void SetSpectatorPawn(class ASpectatorPawn* NewSpectatorPawn) override;
private:
	enum class EPlayerPreTurnState : uint8
	{
		None,
		CameraIntroductionRequested,
		CameraIntroductionPlaying
	};

	void DetermineShotType();

	void DestroyPawn();

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void Init();

	void InitFromConsoleVariables();

	void ResetShotAfterOutOfBounds();

	UFUNCTION(Client, Reliable)
	void ClientHandleOutOfBounds();

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	void SetupNextShot(bool bSetCanFlick);

	void SpectatePawn(APawn* PawnToSpectate, AGolfPlayerState* InPlayerState);

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	UFUNCTION(Client, Reliable)
	void ClientSetTransformTo(const FVector_NetQuantize& Position, const FRotator& Rotation);

	UFUNCTION(Client, Reliable)
	void ClientResetShotAfterOutOfBounds(const FVector_NetQuantize& Position);

	UFUNCTION(Client, Reliable)
	void ClientActivateTurn();

	UFUNCTION(Client, Reliable)
	void ClientStartHole(AActor* InPlayerStart);

	UFUNCTION(Client, Reliable)
	void ClientSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	void DoActivateTurn();

	UFUNCTION(Server, Unreliable)
	void ServerSetPaperGolfPawnRotation(const FRotator& InTotalRotation);

	UFUNCTION(Server, Reliable)
	void ServerProcessShootInput(const FRotator& InTotalRotation);

	float GetAdjustedAccuracy(float Accuracy) const;

	UFUNCTION()
	void OnRep_Scored();

	void OnScored();

	UFUNCTION(BlueprintPure)
	bool CanFlick() const;

	bool ShouldEnableInputForActivateTurn() const;
	
	UFUNCTION(BlueprintPure)
	float GetFlickZ() const;

	virtual AController* AsController() override { return this; }

	// Inherited via IGolfController
	virtual UGolfControllerCommonComponent* GetGolfControllerCommonComponent() override;
	virtual void DoAdditionalOnShotFinished() override;
	virtual void DoAdditionalFallThroughFloor() override;

	void DoReset();
	void EndAnyActiveTurn();

	bool IsLocalClient() const;
	bool IsLocalServer() const;
	bool IsRemoteServer() const;

	bool IsHoleFlybySeen() const;
	void MarkHoleFlybySeen();
	void TriggerPlayerCameraIntroduction();
	void DoPlayerCameraIntroduction();
	void SpectateCurrentGolfHole();
	void MarkFirstPlayerTurnReady();

	bool CameraIntroductionInProgress() const;

	void OnHandleSpectatorShot(AGolfPlayerState* InPlayerState, APaperGolfPawn* InPawn);

	UFUNCTION()
	void OnSpectatorShotPawnSet(APlayerState* InPlayer, APawn* InNewPawn, APawn* InOldPawn);

	UFUNCTION()
	void OnSpectatedPawnDestroyed(AActor* InPawn);

	IPawnCameraLook* GetPawnCameraLook() const;

private:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShotArcPreviewComponent> ShotArcPreviewComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfControllerCommonComponent> GolfControllerCommonComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator RotationMax{ 75.0, 180.0, 90.0 };

	FRotator TotalRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RotationRate{ 100.0f };
	
	/*
	* Increase the value to make slight accuracy errors more forgiving. This is on top of the defaults on the pawn.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta=( ClampMin="1.0" ))
	float AccuracyAdjustmentExponent{ 1.0f };

	/*
	* Decrease to make penalty of worst possible shots less severe.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxAccuracy{ 1.0f };

	float FlickZ{ };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FLinearColor FlickReticuleColor{ };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	int32 FlickZNotUpdatedMaxRetries{ 2 };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float PlayerCameraIntroductionTime{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float PlayerCameraIntroductionBlendExp{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float HoleCameraCutTime{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float HoleCameraCutExponent{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpectatorShotCameraCutTime{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float SpectatorShotCameraCutExponent{ 1.0f };

	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};
	FDelegateHandle OnFlickSpectateShotHandle{};

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	TWeakObjectPtr<AGolfPlayerState> CurrentSpectatorPlayerState{};

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AGolfPlayerState>> SpectatingPawnPlayerStateMap{};

	UPROPERTY(Transient)
	TObjectPtr<AActor> PlayerStart{};

	bool bCanFlick{ };
	bool bTurnActivated{};
	bool bInputEnabled{ true };
	EPlayerPreTurnState PreTurnState{ EPlayerPreTurnState::None };

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Scored)
	bool bScored{};

	bool bOutOfBounds{};

	bool bSpectatorFlicked{};

};

#pragma region Inline Definitions

FORCEINLINE bool AGolfPlayerController::CameraIntroductionInProgress() const
{
	return PreTurnState == EPlayerPreTurnState::CameraIntroductionRequested || PreTurnState == EPlayerPreTurnState::CameraIntroductionPlaying;
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

FORCEINLINE UGolfControllerCommonComponent* AGolfPlayerController::GetGolfControllerCommonComponent()
{
	return GolfControllerCommonComponent;
}

FORCEINLINE bool AGolfPlayerController::IsLocalClient() const
{
	return !HasAuthority() && IsLocalController();
}

FORCEINLINE bool AGolfPlayerController::IsLocalServer() const
{
	return HasAuthority() && IsLocalController();
}

FORCEINLINE bool AGolfPlayerController::IsRemoteServer() const
{
	return HasAuthority() && !IsLocalController();
}

#pragma endregion Inline Definitions
