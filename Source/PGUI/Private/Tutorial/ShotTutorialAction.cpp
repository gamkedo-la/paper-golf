// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/ShotTutorialAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotTutorialAction)


namespace
{
	const TArray<FText> Messages = {
		NSLOCTEXT("ShotTutorialAction", "Message1", "Press LMB to start the shot meter"),
		NSLOCTEXT("ShotTutorialAction", "Message2", "Press LMB again to set the power"),
		NSLOCTEXT("ShotTutorialAction", "Message3", "Press LMB a third time to set accuracy at the black line"),
	};
}

void UShotTutorialAction::Execute()
{
	// TODO: Also mark complete if shot button is pressed and accuracy is decent
	ShowMessages(Messages);
}
