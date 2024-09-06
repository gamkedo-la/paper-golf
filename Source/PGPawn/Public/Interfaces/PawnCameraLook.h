// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PawnCameraLook.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UPawnCameraLook : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PGPAWN_API IPawnCameraLook
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Camera Look")
	virtual void AddCameraRelativeRotation(const FRotator& DeltaRotation) = 0;

	UFUNCTION(BlueprintCallable, Category = "Camera Look")
	virtual void ResetCameraRelativeRotation() = 0;

	UFUNCTION(BlueprintCallable, Category = "Camera Look")
	virtual void AddCameraZoomDelta(float ZoomDelta) = 0;
};
