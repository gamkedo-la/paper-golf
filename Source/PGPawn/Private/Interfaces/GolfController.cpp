// Copyright Game Salutes. All Rights Reserved.


#include "Interfaces/GolfController.h"

#include "State/GolfPlayerState.h"
#include "Pawn/PaperGolfPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfController)


AGolfPlayerState* IGolfController::GetGolfPlayerState()
{
	return AsController()->GetPlayerState<AGolfPlayerState>();
}

const AGolfPlayerState* IGolfController::GetGolfPlayerState() const
{
	return AsController()->GetPlayerState<const AGolfPlayerState>();
}

void IGolfController::AddStroke()
{
	auto GolfPlayerState = GetGolfPlayerState();
	if (!ensure(GolfPlayerState))
	{
		return;
	}

	GolfPlayerState->AddShot();
}
