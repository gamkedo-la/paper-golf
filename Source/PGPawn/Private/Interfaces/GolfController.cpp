// Copyright Game Salutes. All Rights Reserved.


#include "Interfaces/GolfController.h"

#include "State/GolfPlayerState.h"


void IGolfController::AddStroke()
{
	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	GolfPlayerState->AddShot();
}
