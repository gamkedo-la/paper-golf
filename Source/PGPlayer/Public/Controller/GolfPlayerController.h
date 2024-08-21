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

	// Called on both clients and server via a rep notify
	virtual void SetPawn(APawn* InPawn) override;

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintActivateTurn();

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintResetShot();

	virtual void ClientReset_Implementation() override;

private:

	void DetermineShotType();

	void DestroyPawn();

	bool IsFlickedAtRest() const;

	void DrawFlickLocation();

	void ResetFlickZ();

	void Init();

	void ResetShotAfterOutOfBounds();

	UFUNCTION(Client, Reliable)
	void ClientHandleOutOfBounds();

	UFUNCTION()
	void OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn);

	void SetupNextShot(bool bSetCanFlick);

	void AddSpectatorPawn(APawn* PawnToSpectate);
	void SetCameraToViewPawn(APawn* InPawn);

	void SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation = {});

	UFUNCTION(Client, Reliable)
	void ClientSetTransformTo(const FVector_NetQuantize& Position, const FRotator& Rotation);

	UFUNCTION(Client, Reliable)
	void ClientResetShotAfterOutOfBounds(const FVector_NetQuantize& Position);

	UFUNCTION(Client, Reliable)
	void ClientActivateTurn();

	UFUNCTION(Client, Reliable)
	void ClientSpectate(APaperGolfPawn* InPawn, AGolfPlayerState* InPlayerState);

	void DoActivateTurn();

	UFUNCTION(Server, Unreliable)
	void ServerSetPaperGolfPawnRotation(const FRotator& InTotalRotation);

	UFUNCTION(Server, Reliable)
	void ServerProcessShootInput(const FRotator& InTotalRotation);

	UFUNCTION()
	void OnRep_Scored();

	void OnScored();

	UFUNCTION(BlueprintPure)
	bool CanFlick() const;

	AGolfHole* GetGolfHole() const;

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

	float FlickZ{ };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FLinearColor FlickReticuleColor{ };

	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	float OutOfBoundsDelayTime{ 3.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Correction")
	int32 FlickZNotUpdatedMaxRetries{ 2 };

	EShotType ShotType{ EShotType::Default };

	FTimerHandle NextShotTimerHandle{};
	FDelegateHandle OnFlickSpectateShotHandle{};

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
