// Copyright Game Salutes. All Rights Reserved.


#include "Controller/GolfAIController.h"

#include "Components/GolfControllerCommonComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIController)

AGolfAIController::AGolfAIController()
{
	bWantsPlayerState = true;

	GolfControllerCommonComponent = CreateDefaultSubobject<UGolfControllerCommonComponent>(TEXT("GolfControllerCommon"));
}

void AGolfAIController::MarkScored()
{
}

bool AGolfAIController::HasScored() const
{
	return false;
}

bool AGolfAIController::IsActivePlayer() const
{
	return false;
}

bool AGolfAIController::IsReadyForShot() const
{
	return false;
}

void AGolfAIController::ActivateTurn()
{
}

void AGolfAIController::Spectate(APaperGolfPawn* InPawn)
{
}

bool AGolfAIController::HandleOutOfBounds()
{
	return false;
}

APaperGolfPawn* AGolfAIController::GetPaperGolfPawn()
{
	return nullptr;
}


EShotType AGolfAIController::GetShotType() const
{
	return EShotType();
}
