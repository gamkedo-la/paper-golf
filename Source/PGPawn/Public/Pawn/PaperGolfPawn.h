// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PaperGolfPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS(Abstract)
class PGPAWN_API APaperGolfPawn : public APawn
{
	GENERATED_BODY()

public:
	APaperGolfPawn();

	UFUNCTION(BlueprintCallable, meta = (DevelopmentOnly))
	void DebugDrawCenterOfMass(float DrawTime = 0.0f);

	UFUNCTION(BlueprintPure)
	bool IsStuckInPerpetualMotion() const;

	UFUNCTION(BlueprintCallable)
	void SetCloseShot(bool CloseShot);

	UFUNCTION(BlueprintPure)
	bool IsCloseShot() const { return bCloseShot;  }

	UFUNCTION(BlueprintCallable)
	void SetFocusActor(AActor* Focus);

	UFUNCTION(BlueprintCallable)
	void SnapToGround();

	UFUNCTION(BlueprintCallable)
	void ResetRotation();

	UFUNCTION(BlueprintPure)
	FVector GetFlickDirection() const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickLocation(float LocationZ, float Accuracy, float Power) const;

	UFUNCTION(BlueprintPure)
	FVector GetFlickForce(float Accuracy, float Power) const;

	UFUNCTION(BlueprintPure)
	FVector GetLinearVelocity() const;

	UFUNCTION(BlueprintPure)
	FVector GetAngularVelocity() const;

	UFUNCTION(BlueprintPure)
	bool IsAtRest() const;

	UFUNCTION(BlueprintCallable)
	void SetUpForNextShot();

	UFUNCTION(BlueprintCallable)
	void Flick(float LocalZOffset, float PowerFraction, float Accuracy);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;


private:

	float GetFlickMaxForce() const;
	void SetCameraForFlick();
	void ResetCameraForShotSetup();

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	float FlickMaxForce { 330.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	FVector FlickLocation{ 0.0, 0.0, 0.05 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	FRotator InitialRotation{ EForceInit::ForceInitToZero };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	float RestLinearVelocitySquaredMax{ 4.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	float RestAngularVelocityRadsSquaredMax{ 0.001f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> FocusActor{};

	// TODO: Rename - this doesn't make much sense
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	float StandaloneGameMutliplier{ 1.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	float FlickMaxForceCloseShot{ 100.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shot", meta = (AllowPrivateAccess = "true"))
	bool bCloseShot{};

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float PowerAccuracyDampenExp{ 0.25f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty")
	float MinPowerMultiplier{ 0.5f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float PowerAccuracyExp{ 2.0f };

	UPROPERTY(EditDefaultsOnly, Category = "Shot | Difficulty", meta=(ClampMin="0.0"))
	float LocationAccuracyMultiplier{ 2.0f };

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

FORCEINLINE void APaperGolfPawn::SetCloseShot(bool CloseShot)
{
	bCloseShot = CloseShot;
}

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

#pragma endregion Inline Definitions