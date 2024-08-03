// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PaperGolfPawnUtilities.generated.h"

/**
 * 
 */
UCLASS()
class PGPAWN_API UPaperGolfPawnUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Math")
	static void ClampDeltaRotation(const FRotator& MaxRotationExtent, UPARAM(ref) FRotator& DeltaRotation, UPARAM(ref) FRotator& TotalRotation);

	UFUNCTION(BlueprintCallable, Category = "Physics")
	static void ResetPhysicsState(class UPrimitiveComponent* PhysicsComponent, const FTransform& RelativeTransform);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawPoint(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float PointSize = 15.0f);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawSphere(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float Radius, int32 NumSegments = 16);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawBox(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, const FVector& Extent);
};
