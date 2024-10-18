// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/ShotTutorialAction.h"

#include "GameFramework/PlayerController.h"
#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

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

void UShotTutorialAction::Abort()
{
	Super::Abort();
	
	// If the tutorial is aborted but we've shown the messages N times already, mark it as complete
	if (MessagePresentationCount >= MaxMessagePresentations)
	{
		MarkCompleted();
	}
}

void UShotTutorialAction::OnMessageShown(int32 Index, int32 NumMessages)
{
	// Count presentation count once show the first message
	if (Index == 0)
	{
		++MessagePresentationCount;
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: Presentation count %d/%d"), *GetName(), MessagePresentationCount, MaxMessagePresentations);
	}

	Super::OnMessageShown(Index, NumMessages);
}
