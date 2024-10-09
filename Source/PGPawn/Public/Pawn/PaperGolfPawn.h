// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "PaperGolfTypes.h"

#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "Interfaces/PawnCameraLook.h"

#include "PaperGolfPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPaperGolfPawnAudioComponent;
class UPawnCameraLookComponent;

struct FPredictProjectilePathResult;
class UCurveFloat;


USTRUCT()
struct PGPAWN_API FNetworkFlickParams
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FFlickParams FlickParams{};

	UPROPERTY(Transient)
	FRotator Rotation { EForceInit::ForceInitToZero };
};

UCLASS(Abstract)
class PGPAWN_API APaperGolfPawn : public APawn, public IVisualLoggerDebugSnapshotInterface, public IPawnCameraLook
{
	GENERATED_BODY()

public:

	DECLARE_MULTICAST_DELEGATE(FOnFlick);

	APaperGolfPawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

	FOnFlick OnFlick{};

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void DebugDrawCenterOfMass(float DrawTime = 0.0f);

	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
	void SetPawnColor(const FLinearColor& Color);

	UFUNCTION(BlueprintPure)
	bool IsStuckInPerpetualMotion() const;

	void SetFocusActor(AActor* Focus, const TOptional<FVector>& PositionOverride = {});

	float GetRotationYawToFocusActor(AActor* InFocusActor, const TOptional<FVector>& LocationOverride = {}) const;

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ResetRotation();

	UFUNCTION(BlueprintCallable)
	void SetActorHiddenInGameNoRep(bool bInHidden);

	UFUNCTION(BlueprintPure)
	FVector GetFlickDirection() const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickLocation(float LocationZ, float Accuracy = 0.0f, float Power = 1.0f) const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickForce(EShotType ShotType, float Accuracy, float Power) const;

	UFUNCTION(BlueprintPure)
	float GetFlickMaxSpeed(EShotType ShotType) const;

	UFUNCTION(BlueprintPure)
	FVector GetLinearVelocity() const;

	UFUNCTION(BlueprintPure)
	FVector GetAngularVelocity() const;

	UFUNCTION(BlueprintPure)
	bool IsAtRest() const;

	UFUNCTION(BlueprintCallable)
	void SetUpForNextShot();

	UFUNCTION(BlueprintCallable)
	void Flick(const FFlickParams& FlickParams);

	UFUNCTION(BlueprintCallable)
	void AddDeltaRotation(const FRotator& DeltaRotation);

	void SetTransform(const FVector& Position, const TOptional<FRotator>& Rotation = {});

	void SetCollisionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable)
	float ClampFlickZ(float OriginalZOffset, float DeltaZ) const;

	float GetFlickOffsetZTraceSize() const;

	float GetFlickMaxForce(EShotType ShotType) const;

	float GetFlickDragForceMultiplier(float Power) const;

	float GetMass() const;

	void SetReadyForShot(bool bReady);

	bool PredictFlick(const FFlickParams& FlickParams, const FFlickPredictParams& FlickPredictParams, FPredictProjectilePathResult& Result) const;

	UFUNCTION(BlueprintPure)
	AActor* GetFocusActor() const;

	/**
	* Failsafe behavior for handling when the pawn falls below the "Kill-Z".  This should usually be handled by the AFellThroughWorldVolume.
	*/
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	FVector GetPaperGolfPosition() const;
	FRotator GetPaperGolfRotation() const;

	// Overridden for logging purposes - if using for other purposes, move outside the preprocessor guard
#if !UE_BUILD_SHIPPING

	virtual void PostNetInit() override;
	virtual void PostNetReceive() override;
	virtual void OnRep_ReplicatedMovement() override;
	virtual void OnRep_AttachmentReplication() override;

	virtual void PostNetReceiveLocationAndRotation() override;

	/** Update velocity - typically from ReplicatedMovement, not called for simulated physics! */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	/** Update and smooth simulated physic state, replaces PostNetReceiveLocation() and PostNetReceiveVelocity() */
	virtual void PostNetReceivePhysicState() override;

#endif
	// IPawnCameraLook
	virtual void AddCameraRelativeRotation(const FRotator& DeltaRotation) override;
	virtual void ResetCameraRelativeRotation() override;
	virtual void AddCameraZoomDelta(float ZoomDelta) override;

	void ShotFinished();

	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
	void OnTurnStarted();

	virtual ELifetimeCondition AllowActorComponentToReplicate(const UActorComponent* ComponentToReplicate) const;

	UFUNCTION(BlueprintPure)
	USceneComponent* GetPivotComponent() const;

	/*
	* Adjusts the position of the pawn if snap to ground would put the pawn inside the target's radius.
	* If we are already within the target's radius, then we will not adjust the position.
	*/
	bool AdjustPositionIfSnapToGroundOverlapping(const FVector& Target, float TargetRadius);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
	void OnShotFinished();

private:

	bool ShouldReplicateComponent(const UActorComponent* ComponentToReplicate) const;

	void InitDebugDraw();
	void CleanupDebugDraw();

	FVector GetCenterOfMassPosition() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFlick(const FNetworkFlickParams& Params);

	// Used to trigger SFX
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFlick(const FNetworkFlickParams& Params);

	// Making a copy as need to clamp
	void DoFlick(FFlickParams FlickParams);
	void DoNetworkFlick(const FNetworkFlickParams& Params);

#if ENABLE_VISUAL_LOG
	void DrawPawn(const FColor& Color, FVisualLogEntry* Snapshot = nullptr) const;
#endif

	void SetCameraForFlick();
	void ResetCameraForShotSetup(const TOptional<FVector>& PositionOverride = {});

	FNetworkFlickParams ToNetworkParams(const FFlickParams& Params) const;

	void SampleState();

	float CalculateMass() const;

	UFUNCTION()
	void OnRep_FocusActor();

	bool ShouldEnableCameraRotationLagForShotSetup() const;

	void ResetPhysicsState() const;

	void Init();

	float CalculateWidth() const;

private:

#if ENABLE_VISUAL_LOG
	FTimerHandle VisualLoggerTimer{};
#endif

	struct FState
	{
		FVector Position{ EForceInit::ForceInitToZero };

		FState(const APaperGolfPawn& Pawn);
	};

	TArray<FState> States;
	int32 StateIndex{};

	/*
	* Number of samples for checking if stuck in perpetual motion sampled at the tick rate.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Stuck")
	int32 NumSamples{ 25 };

	UPROPERTY(EditDefaultsOnly, Category = "Stuck")
	float MinDistanceThreshold{ 10.0 };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Force")
	float FlickMaxForce { 330.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FVector FlickLocation{ 0.0, 0.0, 0.05 };

	FRotator InitialRotation{ EForceInit::ForceInitToZero };

	FTransform PaperGolfMeshInitialTransform{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RestLinearVelocitySquaredMax{ 4.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RestAngularVelocityRadsSquaredMax{ 0.001f };

	UPROPERTY(Transient, ReplicatedUsing = OnRep_FocusActor)
	TObjectPtr<AActor> FocusActor{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Force")
	float FlickMaxForceCloseShot{ 100.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Force")
	float FlickMaxForceMediumShot{ 200.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Force")
	TObjectPtr<UCurveFloat> FlickDragForceCurve{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta = (ClampMin = "0.0"))
	float PowerAccuracyDampenExp{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinPowerMultiplier{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float PowerAccuracyExp{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot", meta = (ClampMin = "1.0"))
	float FlickOffsetZTraceSize{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Spectator", meta = (ClampMin = "0.0"))
	float SpectatorCameraRotationLag{ 1.0f };

	float OriginalCameraRotationLag{};

	float Mass{};
	float Width{};
	bool bReadyForShot{};

	/*
	* 1 for right handed and -1 for left handed.  Will be set on the pawn by APaperGolfGameModeBase::SpawnDefaultPawnAtTransform_Implementation from player preferences
	* on the player state.
	*/
	int8 HandednessSign : 3 { 1 };

	// TODO: move component set up to C++
private:

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> _PivotComponent{};

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> _PaperGolfMesh{};

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> _FlickReference{};

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> _CameraSpringArm{};

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> _Camera{};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPaperGolfPawnAudioComponent> PawnAudioComponent{};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPawnCameraLookComponent> CameraLookComponent{};
};

#pragma region Inline Definitions

FORCEINLINE FVector APaperGolfPawn::GetLinearVelocity() const
{
	check(_PaperGolfMesh);

	return _PaperGolfMesh->GetPhysicsLinearVelocity();
}

FORCEINLINE FVector APaperGolfPawn::GetAngularVelocity() const
{
	check(_PaperGolfMesh);

	return _PaperGolfMesh->GetPhysicsAngularVelocityInRadians();
}

FORCEINLINE float APaperGolfPawn::GetFlickOffsetZTraceSize() const
{
	return FlickOffsetZTraceSize;
}

FORCEINLINE AActor* APaperGolfPawn::GetFocusActor() const
{
	return FocusActor;
}

FORCEINLINE USceneComponent* APaperGolfPawn::GetPivotComponent() const
{
	return _PivotComponent;
}

FORCEINLINE float APaperGolfPawn::GetMass() const
{
	return Mass;
}

FORCEINLINE float APaperGolfPawn::GetFlickMaxSpeed(EShotType ShotType) const
{
	return GetFlickMaxForce(ShotType) / GetMass();
}

#pragma endregion Inline Definitions
