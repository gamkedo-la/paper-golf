// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/ShotTutorialAction.h"

#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShotTutorialAction)


namespace
{
	// TODO: Need to read which control scheme is in place (Controller, Mouse + Keyboard or just keyboard) and read the data asset to determine the button mappings
	const TArray<FText> Messages = {
		NSLOCTEXT("ShotTutorialAction", "Message1", "Press LMB to start the shot meter"),
		NSLOCTEXT("ShotTutorialAction", "Message2", "Press LMB again to set the power"),
		NSLOCTEXT("ShotTutorialAction", "Message3", "Press LMB a third time to set accuracy at the black line"),
	};
}

// TODO: Also mark complete if shot button is pressed and accuracy is decent

UShotTutorialAction::UShotTutorialAction()
{
	Messages = ::Messages;
}
