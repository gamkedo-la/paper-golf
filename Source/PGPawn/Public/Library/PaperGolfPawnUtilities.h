// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PaperGolfPawnUtilities.generated.h"

class APaperGolfPawn;

/**
 * 
 */
UCLASS()
class PGPAWN_API UPaperGolfPawnUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Math")
	static void ClampDeltaRotation(const FRotator& MinRotationExtent, const FRotator& MaxRotationExtent, UPARAM(ref) FRotator& DeltaRotation, UPARAM(ref) FRotator& TotalRotation);

	static void ResetPhysicsState(class UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform = {});

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawPoint(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float PointSize = 15.0f);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawSphere(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float Radius, int32 NumSegments = 16);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawBox(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, const FVector& Extent);

	static void ReattachPhysicsComponent(class UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform, bool bForceUpdate = false);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool TraceShotAngle(const UObject* WorldContextObject, const APaperGolfPawn* PlayerPawn,
		const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees, float MinTraceDistance = 1000.0f);

	UFUNCTION(BlueprintCallable, Category = "Math")
	static bool TraceCurrentShotWithParameters(const UObject* WorldContextObject, const APaperGolfPawn* PlayerPawn,
		const FFlickParams& FlickParams, float FlickAngleDegrees, float MinTraceDistance = 1000.0f);

private:
	// UFUNCTION does not allow default parameters for non-primitive types so add overload specifically for blueprints
	UFUNCTION(BlueprintCallable, Category = "Physics", meta = (DisplayName = "ResetPhysicsState"))
	static void BlueprintResetPhysicsState(class UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform);
};

#pragma region Inline Definitions

FORCEINLINE void UPaperGolfPawnUtilities::ResetPhysicsState(class UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform)
{
	BlueprintResetPhysicsState(PhysicsComponent, RelativeTransform);
}

#pragma endregion Inline Definitions
