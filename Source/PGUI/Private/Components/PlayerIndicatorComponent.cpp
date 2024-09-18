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
	if (!ensureMsgf(Player, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName()))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: SetVisibleForPlayer: Player is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: SetVisibleForPlayer - Player=%s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player->GetPlayerName());

	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		const auto Text = FText::FromString(GetPlayerIndicatorString(*Player));

		ITextDisplayingWidget::Execute_SetText(TextWidget, Text);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGUI, Error, TEXT("%s-%s: SetVisibleForPlayer: Player=%s Widget is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName(), *Player->GetPlayerName());
	}

	SetVisibility(true);
}

FString UPlayerIndicatorComponent::GetPlayerIndicatorString(const AGolfPlayerState& Player) const
{
	// TODO: It's possible taht the updated shots haven't replicated yet and this could be a stale value
	// Fix for this is to add a listener for the Player stat shots updated event and then update the text if we are visible and if not visible, remove the listener
	// AGolfPlayerState::OnHoleShotsUpdated.AddUObject(this, &UPlayerIndicatorComponent::OnHoleShotsUpdated);
	const auto CurrentHoleShots = Player.GetShots();

	if (CurrentHoleShots > 1)
	{
		return FString::Printf(TEXT("%s - Stroke %d"), *Player.GetPlayerName(), CurrentHoleShots);
	}

	return Player.GetPlayerName();
}

void UPlayerIndicatorComponent::Hide()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s: Hide"), *GetName());

	if (auto TextWidget = GetUserWidgetObject(); TextWidget)
	{
		ITextDisplayingWidget::Execute_SetText(TextWidget, {});
	}

	SetVisibility(false);
}

void UPlayerIndicatorComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGUI, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

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

FVector2D UPlayerIndicatorComponent::ModifyProjectedLocalPosition(const FGeometry& ViewportGeometry, const FVector2D& LocalPosition)
{
	// TODO: Make sure the position is always on screen
	// Cannot actually do that with this function as it is only called if the screen projection would render the position - see SWorldWidgetScreenLayer::Tick
	return Super::ModifyProjectedLocalPosition(ViewportGeometry, LocalPosition);
}
