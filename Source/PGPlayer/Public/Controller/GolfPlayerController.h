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
class UTutorialTrackingSubsystem;
class UTutorialConfigDataAsset;


USTRUCT()
struct FSpectatorParams
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> Pawn{};

	UPROPERTY(Transient)
	TObjectPtr<AGolfPlayerState> PlayerState{};
};

USTRUCT()
struct FTurnActivationClientParams
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FVector Position{ EForceInit::ForceInitToZero };

	UPROPERTY(Transient)
	FRotator WorldRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(Transient)
	FRotator TotalRotation{ EForceInit::ForceInitToZero };
};

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
	virtual bool HasPaperGolfPawn() const override;

	virtual void ReceivePlayerStart(AActor* PlayerStart) override;
	virtual void StartHole(EHoleStartType InHoleStartType) override;
	virtual void ActivateTurn() override;
	virtual void Spectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure)
	virtual bool HasScored() const override;

	virtual void MarkScored() override;

	UFUNCTION(BlueprintPure)
	virtual bool IsActivePlayer() const override;

	UFUNCTION(BlueprintPure)
	virtual bool IsActiveShotInProgress() const override;

	virtual bool HandleHazard(EHazardType HazardType) override;

	UFUNCTION(BlueprintPure)
	virtual EShotType GetShotType() const override;

	virtual void Reset() override;

	virtual void AddPitchInput(float Val) override;

	virtual void AddYawInput(float Val) override;

	virtual bool IsLookInputIgnored() const override { return false; }

	UFUNCTION(BlueprintPure)
	bool IsSpectatingShotSetup() const;

	UFUNCTION(BlueprintPure)
	bool IsInCinematicSequence() const;

	// Redefining to return the explicit type and make it BlueprintCallable

	UFUNCTION(BlueprintCallable)
	AGolfHole* GetCurrentGolfHole() const;

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

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintPlayerReadyForShot(const FRotator& InitialRelativeRotation);

	virtual void ClientReset_Implementation() override;

	void TriggerHoleFlybyAndPlayerCameraIntroduction();
	void OnCameraIntroductionComplete();

	UFUNCTION(BlueprintCallable)
	void SkipHoleFlybyAndCameraIntroduction();

	virtual void SetSpectatorPawn(class ASpectatorPawn* NewSpectatorPawn) override;


#if ENABLE_VISUAL_LOG
	virtual bool ShouldCaptureDebugSnapshot() const override;
#endif
private:
	enum class EPlayerPreTurnState : uint8
	{
		None,
		HoleFlybyPlaying,
		CameraIntroductionRequested,
		CameraIntroductionPlaying
	};

	UFUNCTION()
	void OnHoleComplete();

	void DetermineShotType();

	void DestroyPawn();

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void Init();
	void RegisterEvents();
	void InitFromConsoleVariables();

	void ResetShotAfterHazard();

	UFUNCTION(Client, Reliable)
	void ClientHandleHazard(EHazardType HazardType);

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	void SetupNextShot(bool bSetCanFlick);

	//  Must be called on server for spectator pawn to spawn
	void SpectatePawn(APawn* PawnToSpectate, AGolfPlayerState* InPlayerState);

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	UFUNCTION(Client, Reliable)
	void ClientSetTransformTo(const FVector_NetQuantize& Position, const FRotator& Rotation);

	UFUNCTION(Client, Reliable)
	void ClientResetShotAfterHazard(const FVector_NetQuantize& Position);

	UFUNCTION(Client, Reliable)
	void ClientActivateTurn(const FTurnActivationClientParams& InTurnActivationClientParams);

	UFUNCTION(Client, Reliable)
	void ClientStartHole(AActor* InPlayerStart, EHoleStartType InHoleStartType);

	void DoSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	void DoActivateTurn();
	void ShowActivateTurnHUD();

	UFUNCTION(Server, Unreliable)
	void ServerSetPaperGolfPawnRotation(const FRotator& InTotalRotation);

	UFUNCTION(Server, Reliable)
	void ServerProcessShootInput(const FRotator& InTotalRotation);

	UFUNCTION(Server, Reliable)
	void ServerResetShot();

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
	void TriggerHoleFlyby(const AGolfHole& GolfHole);
	void TriggerPlayerCameraIntroduction();
	void DoPlayerCameraIntroduction();
	void SpectateCurrentGolfHole();
	void MarkFirstPlayerTurnReady();
	void ResetCoursePreviewTrackingState();

	bool CameraIntroductionInProgress() const;

	UFUNCTION(BlueprintPure)
	bool HoleflybyInProgress() const;

	void OnHandleSpectatorShot(AGolfPlayerState* InPlayerState, APaperGolfPawn* InPawn);

	UFUNCTION()
	void OnSpectatorShotPawnSet(APlayerState* InPlayer, APawn* InNewPawn, APawn* InOldPawn);

	UFUNCTION()
	void OnSpectatedPawnDestroyed(AActor* InPawn);

	UFUNCTION()
	void OnHoleFlybySequenceComplete();

	IPawnCameraLook* GetPawnCameraLook() const;

	void SnapCameraBackToPlayer();

	void RegisterHoleFlybyComplete();
	void UnregisterHoleFlybyComplete();

	UTutorialTrackingSubsystem* GetTutorialTrackingSubsystem() const;

	UFUNCTION()
	void OnRep_SpectatorParams();

	// TODO: Do to a timing issue with the RPCs, we have to use this a majority of the time and only
	// use the OnRep_ in the cases where we haven't had a turn yet
	// Otherwise, the pawn attachments are broken on the client and weird stuff happens

	UFUNCTION(Client, Reliable)
	void ClientSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	bool ShouldSpectateFromRep() const;

	void CheckRepDoSpectate();

	void AddPaperGolfPawnRelativeRotationNoInterp(const FRotator& RotationToApply);

	float CalculateIdealPitchAngle() const;
	void ApplyIdealPitchAngle(float PitchAngle);

	void ApplyCurrentTurnActivationClientParams();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Reset Shot"))
	void ResetShotWithServerInvocation();

	void CancelAndHideActiveTutorial();

private:

	UPROPERTY(Category = "Components", VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShotArcPreviewComponent> ShotArcPreviewComponent{};

	UPROPERTY(Category = "Components", VisibleDefaultsOnly)
	TObjectPtr<UGolfControllerCommonComponent> GolfControllerCommonComponent{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator RotationMin{ -75.0, -180.0, -90.0 };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator RotationMax{ 30.0, 180.0, 90.0 };

	FRotator TotalRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RotationRate{ 100.0f };

	/*
	* Increase the value to make slight accuracy errors more forgiving. This is on top of the defaults on the pawn.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "1.0"))
	float AccuracyAdjustmentExponent{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float SpinAccuracyPenaltyFactor{ 0.0f };

	/* Max flick Z displacement. Used for spin accuracy penalty calculations */
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	float MaxFlickZ{ 50.0f };

	/*
	* Decrease to make penalty of worst possible shots less severe.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxAccuracy{ 1.0f };

	float FlickZ{ };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FLinearColor FlickReticuleColor{ };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float DefaultPitchAngle{ 45.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float HazardDelayTime{ 3.0f };

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

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	float TutorialDisplayTurnDelayTime{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Tutorial")
	TObjectPtr<UTutorialConfigDataAsset> TutorialConfigDataAsset{};

	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};
	FTimerHandle CameraIntroductionStartTimerHandle{};
	FTimerHandle TutorialDisplayTurnDelayTimerHandle{};

	FDelegateHandle OnFlickSpectateShotHandle{};

	UPROPERTY(Transient)
	TObjectPtr<APaperGolfPawn> PlayerPawn{};

	UPROPERTY(Transient, ReplicatedUsing = OnRep_SpectatorParams)
	FSpectatorParams SpectatorParams{};

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AGolfPlayerState>> SpectatingPawnPlayerStateMap{};

	UPROPERTY(Transient)
	TObjectPtr<AActor> PlayerStart{};

	TOptional<FTurnActivationClientParams> TurnActivationClientParamsOptional{};

	bool bCanFlick{ };
	bool bTurnActivated{};
	bool bTurnActivationRequested{};
	bool bInputEnabled{ true };
	EPlayerPreTurnState PreTurnState{ EPlayerPreTurnState::None };

	UPROPERTY(Transient, ReplicatedUsing = OnRep_Scored)
	bool bScored{};

	bool bInHazard{};

	bool bSpectatorFlicked{};
	bool bStartedSpectating{};
	EHoleStartType HoleStartType{};
	bool bFirstAction{ true };
};

#pragma region Inline Definitions

FORCEINLINE bool AGolfPlayerController::CameraIntroductionInProgress() const
{
	return PreTurnState == EPlayerPreTurnState::CameraIntroductionRequested || PreTurnState == EPlayerPreTurnState::CameraIntroductionPlaying;
}

FORCEINLINE bool AGolfPlayerController::HoleflybyInProgress() const
{
	return PreTurnState == EPlayerPreTurnState::HoleFlybyPlaying;
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

FORCEINLINE bool AGolfPlayerController::IsActiveShotInProgress() const
{
	return IsActivePlayer() && !CanFlick();
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
