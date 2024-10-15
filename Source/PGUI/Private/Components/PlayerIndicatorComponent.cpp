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

	// TODO: Need to set replicated to avoid excessive warnings: LogNetPackageMap: Warning: FNetGUIDCache::SupportsObject: TextRenderComponent
	// Still get 4 of these at the end of the hole
	SetIsReplicatedByDefault(true);
	// This removes the warning LogNet: Warning: UActorChannel::ProcessBunch: ReadContentBlockPayload failed to find/create object. RepObj: NULL, Channel: 13
	// Not sure where TextRenderComponent is being spawned as WidgetComponent base class does not explicitly spawn it
	bReplicateUsingRegisteredSubObjectList = true;
	SetNetAddressable();
}

void UPlayerIndicatorComponent::SetVisibleForPlayer(AGolfPlayerState* Player)
{
	VisiblePlayer = Player;

	if (!ensureMsgf(Player, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	// Add listener for when strokes are updated
	if (bShowStrokeCounts)
	{
		Player->OnHoleShotsUpdated.AddUObject(this, &UPlayerIndicatorComponent::OnHoleShotsUpdated);
	}

	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: SetVisibleForPlayer - Player=%s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player->GetPlayerName());

	UpdatePlayerIndicatorText(*Player);

	SetVisibility(true);
}

void UPlayerIndicatorComponent::UpdatePlayerIndicatorText(const AGolfPlayerState& Player)
{
	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		const auto Text = FText::FromString(GetPlayerIndicatorString(Player));

		ITextDisplayingWidget::Execute_SetText(TextWidget, Text);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: UpdatePlayerIndicatorText: Player=%s Widget is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player.GetPlayerName());
	}
}

FString UPlayerIndicatorComponent::GetPlayerIndicatorString(const AGolfPlayerState& Player) const
{
	const auto CurrentHoleShots = Player.GetShots();

	if (bShowStrokeCounts && CurrentHoleShots >= MinStrokesToShow)
	{
		return FString::Printf(TEXT("%s - Stroke %d"), *Player.GetPlayerName(), CurrentHoleShots);
	}

	return Player.GetPlayerName();
}

void UPlayerIndicatorComponent::OnHoleShotsUpdated(AGolfPlayerState& Player, int32 PreviousShots)
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: OnHoleShotsUpdated - Player=%s; PreviousShots=%d"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player.GetPlayerName(), PreviousShots);

	UpdatePlayerIndicatorText(Player);
}

void UPlayerIndicatorComponent::Hide()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s: Hide"), *GetName());

	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		ITextDisplayingWidget::Execute_SetText(TextWidget, {});
	}

	if (auto Player = VisiblePlayer.Get(); bShowStrokeCounts && Player)
	{
		Player->OnHoleShotsUpdated.RemoveAll(this);
	}

	VisiblePlayer.Reset();

	SetVisibility(false);
}

void UPlayerIndicatorComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: BeginPlay - bShowStrokeCounts=%s; MinStrokesToShow=%d"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bShowStrokeCounts), MinStrokesToShow);

	Super::BeginPlay();

	// Always render on top
	SetRenderCustomDepth(true);
	SetCustomDepthStencilValue(1);
}

void UPlayerIndicatorComponent::InitializeComponent()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: InitializeComponent"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::InitializeComponent();

	SetVisibility(false);
	SetIsReplicated(true);
}
