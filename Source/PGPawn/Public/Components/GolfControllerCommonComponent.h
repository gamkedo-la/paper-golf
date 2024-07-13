// Copyright Game Salutes. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GolfControllerCommonComponent.generated.h"


/*
* Holds common logic shared by AI and player golf controllers.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PGPAWN_API UGolfControllerCommonComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGolfControllerCommonComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
