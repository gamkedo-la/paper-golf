// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "PaperGolfPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
enum class EShotType : uint8;
struct FPredictProjectilePathResult;
class UCurveFloat;

USTRUCT(BlueprintType)
struct FFlickParams
{
	GENERATED_BODY()

	// Must be marked uproperty in order to replicate in an RPC call

	UPROPERTY(Transient, BlueprintReadWrite)
	EShotType ShotType{};

	UPROPERTY(Transient, BlueprintReadWrite)
	float LocalZOffset{};

	UPROPERTY(Transient, BlueprintReadWrite)
	float PowerFraction{ 1.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float Accuracy{ 0.0f };

	void Clamp();
};

USTRUCT(BlueprintType)
struct FFlickPredictParams
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadWrite)
	float MaxSimTime{ 30.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float SimFrequency{ 30.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float CollisionRadius{ 3.0f };
};

bool operator ==(const FFlickParams& First, const FFlickParams& Second);

USTRUCT()
struct FNetworkFlickParams
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	FFlickParams FlickParams{};

	UPROPERTY(Transient)
	FRotator Rotation { EForceInit::ForceInitToZero };
};

UCLASS(Abstract)
class PGPAWN_API APaperGolfPawn : public APawn, public IVisualLoggerDebugSnapshotInterface
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

	UFUNCTION(BlueprintCallable)
	void SetFocusActor(AActor* Focus);

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ResetRotation();

	UFUNCTION(BlueprintPure)
	FVector GetFlickDirection() const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickLocation(float LocationZ, float Accuracy = 0.0f, float Power = 1.0f) const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickForce(EShotType ShotType, float Accuracy, float Power) const;

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

	// Cannot use TOptional in a UFUNCTION
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReliableSetTransform(const FVector_NetQuantize& Position, bool bSnapToGround, bool bUseRotation = false, const FRotator& Rotation = FRotator::ZeroRotator);

	void SetTransform(const FVector& Position, const TOptional<FRotator>& Rotation = {});

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetCollisionEnabled(bool bEnabled);

	void SetCollisionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable)
	float ClampFlickZ(float OriginalZOffset, float DeltaZ) const;

	float GetFlickOffsetZTraceSize() const;

	float GetFlickMaxForce(EShotType ShotType) const;

	float GetFlickDragForceMultiplier(float Power) const;

	float GetMass() const;

	// Used for server validation
	void SetReadyForShot(bool bReady) { bReadyForShot = bReady; }

	bool PredictFlick(const FFlickParams& FlickParams, const FFlickPredictParams& FlickPredictParams, FPredictProjectilePathResult& Result) const;

	UFUNCTION(BlueprintPure)
	AActor* GetFocusActor() const;

	/**
	* Failsafe behavior for handling when the pawn falls below the "Kill-Z".  This should usually be handled by the AFellThroughWorldVolume.
	*/
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	FVector GetPaperGolfPosition() const;
	FRotator GetPaperGolfRotation() const;

	virtual void PostNetReceive() override;
	virtual void OnRep_ReplicatedMovement() override;

	virtual void PostNetReceiveLocationAndRotation() override;

	/** Update velocity - typically from ReplicatedMovement, not called for simulated physics! */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	/** Update and smooth simulated physic state, replaces PostNetReceiveLocation() and PostNetReceiveVelocity() */
	virtual void PostNetReceivePhysicState() override;

	void AddCameraRelativeRotation(const FRotator& DeltaRotation);
	void ResetCameraRelativeRotation();
	void AddCameraZoomDelta(float ZoomDelta);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

private:

	void InitDebugDraw();
	void CleanupDebugDraw();

	FVector GetCenterOfMassPosition() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFlick(const FNetworkFlickParams& Params);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFlick(const FNetworkFlickParams& Params);

	// Making a copy as need to clamp
	void DoFlick(FFlickParams FlickParams);
	void DoNetworkFlick(const FNetworkFlickParams& Params);

#if ENABLE_VISUAL_LOG
	void DrawPawn(const FColor& Color, FVisualLogEntry* Snapshot = nullptr) const;
#endif

	void SetCameraForFlick();
	void ResetCameraForShotSetup();

	FNetworkFlickParams ToNetworkParams(const FFlickParams& Params) const;

	void SampleState();

	float CalculateMass() const;
	void RefreshMass();

	UFUNCTION()
	void OnRep_FocusActor();

	bool ShouldEnableCameraRotationLagForShotSetup() const;

	void ResetPhysicsState() const;

	void InitializePhysicsState();

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
	FRotator InitialSpringArmRotation{ EForceInit::ForceInitToZero };

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

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float PowerAccuracyDampenExp{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float MinPowerMultiplier{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float PowerAccuracyExp{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float LocationAccuracyMultiplier{ 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta = (ClampMin = "1.0"))
	float FlickOffsetZTraceSize{ 5.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot")
	FRotator MinCameraRotation{ -45.0f, -60.0f, 0 };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot")
	FRotator MaxCameraRotation{ 45.0f, 60.0f, 0 };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot", meta = (ClampMin = "0.0"))
	float MinCameraSpringArmLength{ 0.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Shot", meta = (ClampMin = "0.0"))
	float MaxCameraSpringArmLength{ 2000.0f };

	float InitialCameraSpringArmLength{};

	UPROPERTY(EditDefaultsOnly, Category = "Camera | Spectator", meta = (ClampMin = "0.0"))
	float SpectatorCameraRotationLag{ 1.0f };

	float OriginalCameraRotationLag{};

	mutable float Mass{};
	bool bReadyForShot{};

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

FORCEINLINE void APaperGolfPawn::RefreshMass()
{
	Mass = CalculateMass();
}

FORCEINLINE AActor* APaperGolfPawn::GetFocusActor() const
{
	return FocusActor;
}

FORCEINLINE bool operator ==(const FFlickParams& First, const FFlickParams& Second)
{
	return First.ShotType == Second.ShotType &&
		FMath::IsNearlyEqual(First.LocalZOffset, Second.LocalZOffset) &&
		FMath::IsNearlyEqual(First.PowerFraction, Second.PowerFraction) &&
		FMath::IsNearlyEqual(First.Accuracy, Second.Accuracy);
}

#pragma endregion Inline Definitions
