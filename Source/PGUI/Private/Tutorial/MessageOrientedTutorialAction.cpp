// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Tutorial/MessageOrientedTutorialAction.h"

#include "VisualLogger/VisualLogger.h"

#include "PGUILogging.h"

#include "Logging/LoggingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MessageOrientedTutorialAction)


void UMessageOrientedTutorialAction::Execute()
{
	if (ensure(!Messages.IsEmpty()))
	{
		Super::Execute();
		ShowMessages(Messages);
	}
	else
	{
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Error, TEXT("%s: No messages to show"), *GetName());
		MarkCompleted();
	}
}

void UMessageOrientedTutorialAction::Abort()
{
	Super::Abort();

	// If the tutorial is aborted but we've shown the messages N times already, mark it as complete
	if (MessagePresentationCount >= MaxMessagePresentations)
	{
		MarkCompleted();
	}
}

void UMessageOrientedTutorialAction::OnMessageShown(int32 Index, int32 NumMessages)
{
	// Count presentation count once show the first message
	if (Index == 0)
	{
		++MessagePresentationCount;
		UE_VLOG_UELOG(GetOuter(), LogPGUI, Log, TEXT("%s: Presentation count %d/%d"), *GetName(), MessagePresentationCount, MaxMessagePresentations);
	}

	Super::OnMessageShown(Index, NumMessages);
}
