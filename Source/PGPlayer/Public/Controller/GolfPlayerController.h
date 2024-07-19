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
class UGolfControllerCommonComponent;

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
	void ResetShot();

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
	virtual void Spectate(APaperGolfPawn* InPawn) override;

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

	// TODO: Can we remove UFUNCTION on some of these
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	void ClientSpectate(APaperGolfPawn* InPawn);

	void DoActivateTurn();

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

	bool IsLocalClient() const;
	bool IsLocalServer() const;
	bool IsRemoteServer() const;

private:
	
#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

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
