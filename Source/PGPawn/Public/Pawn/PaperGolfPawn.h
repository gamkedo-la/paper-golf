// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "PaperGolfPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
enum class EShotType : uint8;

USTRUCT(BlueprintType)
struct FFlickParams
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	EShotType ShotType{};

	// Must be marked uproperty in order to replicate in an RPC call
	UPROPERTY(Transient)
	float LocalZOffset{};

	UPROPERTY(Transient)
	float PowerFraction{};

	UPROPERTY(Transient)
	float Accuracy{};
};

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
	APaperGolfPawn();

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void DebugDrawCenterOfMass(float DrawTime = 0.0f);

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
	FVector GetFlickLocation(float LocationZ, float Accuracy = 1.0f, float Power = 1.0f) const;

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
	float ClampFlickZ(float OriginalZOffset, float DeltaZ) const;

	float GetFlickOffsetZTraceSize() const;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

private:

	UFUNCTION(Server, Reliable)
	void ServerFlick(const FNetworkFlickParams& Params);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFlick(const FNetworkFlickParams& Params);

	void DoFlick(const FFlickParams& FlickParams);
	void DoNetworkFlick(const FNetworkFlickParams& Params);

#if ENABLE_VISUAL_LOG
	void DrawPawn(FVisualLogEntry* Snapshot) const;
#endif

	float GetFlickMaxForce(EShotType ShotType) const;
	void SetCameraForFlick();
	void ResetCameraForShotSetup();


	FNetworkFlickParams ToNetworkParams(const FFlickParams& Params) const;

private:

	void SampleState();

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

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float FlickMaxForce { 330.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FVector FlickLocation{ 0.0, 0.0, 0.05 };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	FRotator InitialRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RestLinearVelocitySquaredMax{ 4.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float RestAngularVelocityRadsSquaredMax{ 0.001f };

	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusActor{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot")
	float FlickMaxForceCloseShot{ 100.f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float PowerAccuracyDampenExp{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float MinPowerMultiplier{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float PowerAccuracyExp{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float LocationAccuracyMultiplier{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta = (ClampMin = "1.0"))
	float FlickOffsetZTraceSize{ 5.0f };

	// TODO: move component set up to C++
private:
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

#pragma endregion Inline Definitions