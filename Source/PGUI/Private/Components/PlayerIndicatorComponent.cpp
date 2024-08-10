// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/PlayerIndicatorComponent.h"

#include "State/GolfPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGUILogging.h"

#include "UI/Widget/TextDisplayingWidget.h"

#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerIndicatorComponent)


UPlayerIndicatorComponent::UPlayerIndicatorComponent()
{
	bWantsInitializeComponent = true;
}

void UPlayerIndicatorComponent::SetVisibleForPlayer(AGolfPlayerState* Player)
{
	if (!ensureMsgf(Player, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: SetVisibleForPlayer - Player=%s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player->GetPlayerName());

	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		const auto Text = FText::FromString(Player->GetPlayerName());

		ITextDisplayingWidget::Execute_SetText(TextWidget, Text);

		//UE_VLOG_LOCATION(GetOwner(), LogPGUI, Log, GetOwner()->GetActorLocation(), 25.0f, FColor::Green, TEXT("Show Indicator: %s"), *Text.ToString());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: SetVisibleForPlayer: Player=%s Widget is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player->GetPlayerName());
	}

	SetVisibility(true);
}

void UPlayerIndicatorComponent::Hide()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s: Hide"), *GetName());
	//UE_VLOG_LOCATION(GetOwner(), LogPGUI, Log, GetOwner()->GetActorLocation(), 25.0f, FColor::Orange, TEXT("Hide Indicator"));

	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		ITextDisplayingWidget::Execute_SetText(TextWidget, {});
	}

	SetVisibility(false);
}

void UPlayerIndicatorComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: InitializeComponent"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::InitializeComponent();

	SetVisibility(false);
}
