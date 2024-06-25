// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PaperGolfUtilities.generated.h"

/**
 * 
 */
UCLASS()
class PAPERGOLF_API UPaperGolfUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Math")
	static void ClampDeltaRotation(const FRotator& MaxRotationExtent, UPARAM(ref) FRotator& DeltaRotation, UPARAM(ref) FRotator& TotalRotation);


	UFUNCTION(BlueprintCallable, Category = "Physics")
	static void ResetPhysicsState(class UPrimitiveComponent* PhysicsComponent);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static void DrawPoint(const UObject* WorldContextObject, const FVector& Position, const FLinearColor& Color, float PointSize = 15.0f);
};
