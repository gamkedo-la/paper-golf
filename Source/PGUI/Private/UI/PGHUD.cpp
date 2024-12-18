// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "UI/PGHUD.h"

#include "Logging/LoggingUtils.h"
#include "PGUILogging.h"
#include "Utils/ObjectUtils.h"
#include "Utils/ArrayUtils.h"

#include "Utils/PGAudioUtilities.h"

#include "Blueprint/UserWidget.h"

#include "Runtime/CoreUObject/Public/UObject/SoftObjectPtr.h"

#include "Subsystems/GolfEventsSubsystem.h"
#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Pawn/PaperGolfPawn.h"

#include "UI/Widget/TextDisplayingWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGHUD)

void APGHUD::ShowHUD()
{
	Super::ShowHUD();

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: ShowHUD: %s"), *GetName(), LoggingUtils::GetBoolString(bShowHUD));

	OnToggleHUDVisibility(bShowHUD);
}

void APGHUD::BeginPlay()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	Init();
}

void APGHUD::SetHUDVisible(bool bVisible)
{
	if (bVisible == bShowHUD)
	{
		return;
	}

	// Toggles the HUD
	ShowHUD();
}

void APGHUD::DisplayMessageWidget(EMessageWidgetType MessageType)
{
	switch (MessageType)
	{
	case EMessageWidgetType::HoleFinished:
		DisplayHoleFinishedMessage();
		break;
	case EMessageWidgetType::Tutorial:
		DisplayMessageWidgetByClass(TutorialWidgetClass);
		break;
	default:
		checkNoEntry();
	}
}

void APGHUD::RemoveMessageWidget(EMessageWidgetType MessageType)
{
	switch (MessageType)
	{
	case EMessageWidgetType::HoleFinished:
	{
		RemoveHoleFinishedMessage();
		UnregisterHoleFinishedMessageRemoval();
		break;
	}
	case EMessageWidgetType::Tutorial:
	{
		RemoveActiveMessageWidgetByClass(TutorialWidgetClass);
		break;
	}
	default:
		checkNoEntry();
	}
}

void APGHUD::DisplayMessageWidgetWithText(const FText& Message)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayMessageWidgetWithText: Message=%s"), *GetName(), *Message.ToString());
	DisplayMessageWidgetByClass(GenericMessagingWidgetClass, Message);
}

void APGHUD::RemoveMessageWidgetWithText(const FText& Message)
{
	if (!IsValid(ActiveMessageWidget))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveMessageWidgetWithText: Message=%s - Skip: No active message widget"),
			*GetName(), *Message.ToString());
		return;
	}

	if (ActiveMessageWidget->GetClass() != GenericMessagingWidgetClass.Get())
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveMessageWidgetWithText: Message=%s - Skip: Active message widget=%s is not of class %s"),
			*GetName(), *Message.ToString(), *LoggingUtils::GetName(ActiveMessageWidget), *LoggingUtils::GetName(GenericMessagingWidgetClass));
		return;
	}

	if(!Message.EqualTo(LastMessageText))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveMessageWidgetWithText: Message=%s - Skip: Active message widget=%s with message=%s does not match message input"),
			*GetName(), *Message.ToString(), *LoggingUtils::GetName(ActiveMessageWidget), *LastMessageText.ToString());
		return;
	}

	DoRemoveActiveMessageWidgetFromScreen();
}

void APGHUD::DisplayHazardEntryWidget(EHazardType HazardType)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayHazardEntryWidget: HazardType=%s"), *GetName(),
		*LoggingUtils::GetName(HazardType));

	switch (HazardType)
	{
	case EHazardType::OutOfBounds:
		DisplayMessageWidgetByClass(OutOfBoundsWidgetClass);
		break;
	case EHazardType::Water:
		DisplayMessageWidgetByClass(WaterHazardWidgetClass);
		break;
	default:
		checkNoEntry();
	}
}

void APGHUD::DisplayHoleFinishedMessage()
{
	DisplayMessageWidgetByClass(HoleFinishedWidgetClass);
	RegisterHoleFinishedMessageRemoval();
}

void APGHUD::RegisterHoleFinishedMessageRemoval()
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	World->GetTimerManager().SetTimer(HoleFinishedMessageTimerHandle, this, &ThisClass::RemoveHoleFinishedMessage, HoleFinishedMessageDuration);
}

void APGHUD::RemoveHoleFinishedMessage()
{
	if (RemoveActiveMessageWidgetByClass(HoleFinishedWidgetClass))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveHoleFinishedMessage"), *GetName());
	}
}

bool APGHUD::RemoveActiveMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass)
{
	// Make sure the active message widget is still the indicated widget
	// WidgetClass will already be loaded and active if it is dispalyed

	if (!IsValid(ActiveMessageWidget) || ActiveMessageWidget->GetClass() != WidgetClass.Get())
	{
		return false;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveActiveMessageWidgetByClass: WidgetClass=%s"), *GetName(),
		*LoggingUtils::GetName(WidgetClass));

	RemoveActiveMessageWidget();
	
	return true;
}

void APGHUD::UnregisterHoleFinishedMessageRemoval()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(HoleFinishedMessageTimerHandle);
}

void APGHUD::RemoveActiveMessageWidget()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveActiveMessageWidget: ActiveMessageWidget=%s"),
		*GetName(), *LoggingUtils::GetName(ActiveMessageWidget));

	if (DoRemoveActiveMessageWidgetFromScreen())
	{
		UnregisterHoleFinishedMessageRemoval();
	}
}

bool APGHUD::DoRemoveActiveMessageWidgetFromScreen()
{
	if (!IsValid(ActiveMessageWidget))
	{
		return false;
	}

	ActiveMessageWidget->RemoveFromParent();
	ActiveMessageWidget = nullptr;
	LastMessageText = FText::GetEmpty();

	return true;
}

void APGHUD::SpectatePlayer(int32 PlayerStateId)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: SpectatePlayer: PlayerStateId=%d"), *GetName(), PlayerStateId);

	AGolfPlayerState* PlayerState{};
	if (TryFindPlayerById(PlayerStateId, PlayerState))
	{
		ActivePlayer = *PlayerState;
		SpectatePlayer(PlayerState);
	}
	else
	{
		ActivePlayer = FActivePlayer(PlayerStateId, EDeferredPlayerState::SpectatingShotSetup);
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: SpectatePlayer: PlayerStateId=%d - Player not yet replicated - deferring SpectatingShotSetup"),
			*GetName(), PlayerStateId);
	}
}

void APGHUD::SpectatePlayer_Implementation(const AGolfPlayerState* InPlayerState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log,
		TEXT("%s: SpectatePlayer: InPlayerState=%s"), *GetName(), *LoggingUtils::GetName<APlayerState>(InPlayerState));

	if (auto GolfGameState = GetGameState(); GolfGameState)
	{
		CheckNotifyHoleShotsUpdate(*GolfGameState);
	}

	if (ShouldShowActiveTurnWidgets())
	{
		if (!InPlayerState)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: SpectatePlayer: No player state defined"),
				*GetName());
		}

		DisplayTurnWidget(SpectatingWidgetClass, &ThisClass::SpectatingWidget,
			FText::FromString(FString::Printf(TEXT("Spectating %s"), InPlayerState ? *InPlayerState->GetPlayerName() : TEXT(""))));
	}
}

void APGHUD::BeginTurn_Implementation()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginTurn"), *GetName());

	if (auto PC = GetOwningPlayerController(); PC)
	{
		if (auto MyPlayerState = PC->GetPlayerState<AGolfPlayerState>(); ensure(MyPlayerState))
		{
			ActivePlayer = *MyPlayerState;
		}
		else
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Error, TEXT("%s: BeginTurn - Could not get own AGolfPlayerState : %s"),
				*GetName(), *LoggingUtils::GetName(PC->GetPlayerState<APlayerState>()));
			ActivePlayer.Reset();
		}

		if (auto GolfGameState = GetGameState(); GolfGameState)
		{
			CheckNotifyHoleShotsUpdate(*GolfGameState);
		}
	}

	if (ShouldShowActiveTurnWidgets())
	{
		DisplayTurnWidget(ActiveTurnWidgetClass, &ThisClass::ActiveTurnWidget, FText::FromString("Your Turn"));
	}
}

void APGHUD::BeginShot_Implementation()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginShot"), *GetName());

	HideActiveTurnWidget();
}

void APGHUD::HideActiveTurnWidget()
{
	if (IsValid(ActivePlayerTurnWidget))
	{
		ActivePlayerTurnWidget->RemoveFromParent();
		ActivePlayerTurnWidget = nullptr;
		ActivePlayerTurnWidgetClass = nullptr;
	}
}

void APGHUD::BeginSpectatorShot(int32 PlayerStateId)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginSpectatorShot: PlayerStateId=%d"), *GetName(), PlayerStateId);

	AGolfPlayerState* PlayerState{};
	if (TryFindPlayerById(PlayerStateId, PlayerState))
	{
		ActivePlayer = *PlayerState;
		BeginSpectatorShot(PlayerState);
	}
	else
	{
		ActivePlayer = FActivePlayer(PlayerStateId, EDeferredPlayerState::SpectatingShot);
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: SpectatePlayer: PlayerStateId=%d - Player not yet replicated - deferring SpectatingShot"),
			*GetName(), PlayerStateId);
	}
}

void APGHUD::BeginSpectatorShot_Implementation(const AGolfPlayerState* InPlayerState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginSpectatorShot: InPlayerState=%s"),
		*GetName(), *LoggingUtils::GetName<APlayerState>(InPlayerState));

	HideActiveTurnWidget();
}

bool APGHUD::ShouldShowActiveTurnWidgets() const
{
	// Don't show for single player

	if (auto GolfGameState = GetGameState(); GolfGameState)
	{
		// If a multiplayer game started but others players dropped there may be more than 1 in the PlayerArray
		// or if there are spectators only
		// However, in a multiplayer context still show the widget since it would have shown before in the same game
		return GolfGameState->PlayerArray.Num() > 1;
	}

	return false;
}

APaperGolfGameStateBase* APGHUD::GetGameState() const
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return nullptr;
	}

	auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>();
	ensure(GolfGameState);

	return GolfGameState;
}

bool APGHUD::TryFindPlayerById(int32 PlayerStateId, AGolfPlayerState*& OutPlayerState) const
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return false;
	}

	auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>();
	if (!GolfGameState)
	{
		return false;
	}

	OutPlayerState = GolfGameState->GetPlayerStateById(PlayerStateId);
	return OutPlayerState != nullptr;
}

void APGHUD::CheckExecuteDeferredSpectatorAction(const AGolfPlayerState& AddedPlayerState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckExecuteDeferredSpectatorAction: AddedPlayerState=%s"),
		*GetName(), *LoggingUtils::GetName<APlayerState>(AddedPlayerState));

	if (!ActivePlayer || !ActivePlayer->MatchesDeferredState(AddedPlayerState.GetPlayerId()))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckExecuteDeferredSpectatorAction: No deferred state to execute"),
			*GetName());
		return;
	}

	check(ActivePlayer);

	const auto DeferredStateToExecute = ActivePlayer->LastDeferredState;
	ActivePlayer = AddedPlayerState;

	switch (ActivePlayer->LastDeferredState)
	{
		case EDeferredPlayerState::SpectatingShotSetup:
			SpectatePlayer(&AddedPlayerState);
			break;
		case EDeferredPlayerState::SpectatingShot:
			BeginSpectatorShot(&AddedPlayerState);
			break;
		default:
			checkNoEntry();
	}
}

void APGHUD::DisplayMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass, FText MessageToSet)
{
	if (!ensure(!WidgetClass.IsNull()))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayMessageWidgetByClass: %s - MessageToSet=%s"), *GetName(), *LoggingUtils::GetName(WidgetClass), *MessageToSet.ToString());

	ActiveMessageWidgetClass = WidgetClass;

	// Request to load the asset asynchronously
	PG::ObjectUtils::LoadClassAsync<UUserWidget>(WidgetClass, [this, WeakThis = TWeakObjectPtr<APGHUD>(this), WidgetClass, MessageToSet = MoveTemp(MessageToSet)](UClass* LoadedClass)
	{
		if (auto StrongThis = WeakThis.Get(); !StrongThis)
		{
			return;
		}

		// Safe to reference "this"
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s Loaded"), *GetName(), 
			*LoggingUtils::GetName(WidgetClass));

		if (WidgetClass != ActiveMessageWidgetClass)
		{
			// Widget class switched while loading
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Display, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s no longer matches new active widget class=%s"),
				*GetName(), *LoggingUtils::GetName(WidgetClass), *LoggingUtils::GetName(ActiveMessageWidgetClass));
			return;
		}

		if (!LoadedClass)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s could not be loaded"),
				*GetName(), *LoggingUtils::GetName(WidgetClass));
			return;
		}

		auto PC = GetOwningPlayerController();
		if (!PC)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s - Player Controller is NULL"),
				*GetName(), *LoggingUtils::GetName(WidgetClass));
			return;
		}

		const auto NewWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
		if (!NewWidget)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s - Could not create widget"),
				*GetName(), *LoggingUtils::GetName(LoadedClass));
			return;
		}

		if (!MessageToSet.IsEmpty())
		{
			if (NewWidget->GetClass()->ImplementsInterface(UTextDisplayingWidget::StaticClass()))
			{
				SetWidgetText(*NewWidget, MessageToSet);
			}
			else
			{
				UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: DisplayMessageWidgetByClass: WidgetClass=%s; MessageToSet=%s - Widget does not implement ITextDisplayingWidget"),
					*GetName(), *LoggingUtils::GetName(LoadedClass), *MessageToSet.ToString());
				return;
			}
		}

		RemoveActiveMessageWidget();

		ActiveMessageWidget = NewWidget;

		if (!ActiveMessageWidget->IsInViewport())
		{
			ActiveMessageWidget->AddToViewport();
		}
	});
}

void APGHUD::SetWidgetText(UUserWidget& Widget, const FText& Text)
{
	ITextDisplayingWidget::Execute_SetText(&Widget, Text);
	LastMessageText = Text;
}

void APGHUD::LoadWidgetAsync(const TSoftClassPtr<UUserWidget>& WidgetClass, TFunction<void(UUserWidget&)> OnWidgetReady)
{
	if (!ensure(!WidgetClass.IsNull()))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: LoadWidgetAsync: %s"), *GetName(), *LoggingUtils::GetName(WidgetClass));

	// Request to load the asset asynchronously
	PG::ObjectUtils::LoadClassAsync<UUserWidget>(WidgetClass, [this, WeakThis = TWeakObjectPtr<APGHUD>(this), WidgetClass, OnWidgetReady](UClass* LoadedClass)
	{
		if (auto StrongThis = WeakThis.Get(); !StrongThis)
		{
			return;
		}

		// Safe to reference "this"
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: LoadWidgetAsync: WidgetClass=%s Loaded"), *GetName(),
			*LoggingUtils::GetName(WidgetClass));

		if (!LoadedClass)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: LoadWidgetAsync: WidgetClass=%s could not be loaded"),
				*GetName(), *LoggingUtils::GetName(WidgetClass));
			return;
		}

		auto PC = GetOwningPlayerController();
		if (!PC)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: LoadWidgetAsync: WidgetClass=%s - Player Controller is NULL"),
				*GetName(), *LoggingUtils::GetName(WidgetClass));
			return;
		}

		const auto NewWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
		if (!NewWidget)
		{
			UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Warning, TEXT("%s: LoadWidgetAsync: WidgetClass=%s - Could not create widget"),
				*GetName(), *LoggingUtils::GetName(LoadedClass));
			return;
		}

		OnWidgetReady(*NewWidget);
	});
}

void APGHUD::DisplayTurnWidget(const TSoftClassPtr<UUserWidget>& WidgetClass, WidgetMemberPtr WidgetToDisplay, const FText& Message)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayTurnWidget: WidgetClass=%s; WidgetToDisplay=%s; Message=%s"),
		*GetName(), *LoggingUtils::GetName(WidgetClass), *LoggingUtils::GetName(this->*WidgetToDisplay), *Message.ToString());

	auto BoundWidgetToDisplay = this->*WidgetToDisplay;

	if (IsValid(BoundWidgetToDisplay))
	{
		if (ActivePlayerTurnWidget != BoundWidgetToDisplay)
		{
			HideActiveTurnWidget();

			ActivePlayerTurnWidget = BoundWidgetToDisplay;
			ActivePlayerTurnWidgetClass = WidgetClass;
		}

		SetWidgetText(*BoundWidgetToDisplay, Message);
	}
	else
	{
		HideActiveTurnWidget();

		ActivePlayerTurnWidgetClass = WidgetClass;
		LoadWidgetAsync(WidgetClass, [this, WidgetToDisplay, WidgetClass, Message](UUserWidget& NewWidget)
		{
			// LoadWidgetAsync automatically captures this weakly so if this executes we know that the HUD object is still valid
			// Only display if the active widget class is still the same
			if (ActivePlayerTurnWidgetClass != WidgetClass)
			{
				UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Display, TEXT("%s: DisplayTurnWidget: WidgetClass=%s no longer matches new active widget class=%s"),
					*GetName(), *LoggingUtils::GetName(WidgetClass), *LoggingUtils::GetName(ActivePlayerTurnWidgetClass));
				return;
			}

			this->*WidgetToDisplay = &NewWidget;
			SetWidgetText(NewWidget, Message);

			ActivePlayerTurnWidget = &NewWidget;
		});
	}
}

void APGHUD::PlaySound2D(const TSoftObjectPtr<USoundBase>& Sound)
{
	if (!ensure(!Sound.IsNull()))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: PlaySound2D: Requesting Sound=%s"), *GetName(), *Sound.ToString());

	PG::ObjectUtils::LoadObjectAsync<USoundBase>(Sound, [WeakSelf = TWeakObjectPtr<APGHUD>(this)](USoundBase* Sound)
	{
		if (auto Self = WeakSelf.Get(); Self)
		{
			UPGAudioUtilities::PlaySfx2D(Self, Sound);
		}
	});
}

bool APGHUD::FinalResultsAreDetermined() const
{
	return bScoresSynced && bCourseComplete;
}

void APGHUD::CheckForInitialDeferredState(const APaperGolfGameStateBase& GameState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckForInitialDeferredState"), *GetName());

	if (!ActivePlayer || !ActivePlayer->HasAnyDeferredState())
	{
		return;
	}

	for (auto PlayerState : GameState.GetActiveGolfPlayerStates())
	{
		CheckExecuteDeferredSpectatorAction(*PlayerState);
		if (!ActivePlayer)
		{
			return;
		}
	}
}

bool APGHUD::ShouldShowAnyScores() const
{
	const auto GameState = GetGameState();
	
	if (!ensure(GameState))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Error, TEXT("%s: ShouldShowAnyScores - FALSE - GameState is NULL"), *GetName());
		return false;
	}

	if (!GameState->ShouldShowScoresHUD())
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: ShouldShowAnyScores - FALSE - Should not show scores HUD"), *GetName());
		return false;
	}

	return true;
}

void APGHUD::PlayCourseResultsSoundIfApplicable()
{
	const auto bShouldCheckPlaySound = FinalResultsAreDetermined();

	auto PC = GetOwningPlayerController();
	if (!ensure(PC))
	{
		return;
	}

	UE_VLOG_UELOG(PC, LogPGUI, Log, TEXT("%s: PlayCourseResultsSoundIfApplicable - %s: bScoresSynced=%s; bCourseComplete=%s"),
		*GetName(), LoggingUtils::GetBoolString(bShouldCheckPlaySound), LoggingUtils::GetBoolString(bScoresSynced), LoggingUtils::GetBoolString(bCourseComplete));

	if (!bShouldCheckPlaySound)
	{
		return;
	}

	// See if we are the winner
	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto GameState = World->GetGameState<APaperGolfGameStateBase>();
	if (!ensure(GameState))
	{
		return;
	}

	TArray<int32> PlayerRanks;

	const auto& Players = GameState->GetSortedPlayerStatesByScore(&PlayerRanks);

	// Skip win/lose sounds if playing single player
	if(Players.Num() <= 1)
	{
		UE_VLOG_UELOG(PC, LogPGUI, Log, TEXT("%s - PlayerController=%s - Skipping win sound as there is only %d player%s : %s"),
			*GetName(), *PC->GetName(), Players.Num(), LoggingUtils::Pluralize(Players.Num()), *PG::ToStringObjectElements(Players));
		return;
	}

	const auto MyPlayerState = PC->GetPlayerState<AGolfPlayerState>();

	if (!ensureMsgf(MyPlayerState, TEXT("%s - PlayerController=%s - Could not get AGolfPlayerState : %s"),
		*GetName(), *PC->GetName(), *LoggingUtils::GetName<APlayerState>(PC->GetPlayerState<APlayerState>())))
	{
		UE_VLOG_UELOG(PC, LogPGUI, Error, TEXT("%s - PlayerController=%s - Could not get AGolfPlayerState : %s"),
			*GetName(), *PC->GetName(), *LoggingUtils::GetName<APlayerState>(PC->GetPlayerState<APlayerState>()));
		return;
	}

	// Skip playing sound if spectator
	if (MyPlayerState->IsSpectatorOnly())
	{
		UE_VLOG_UELOG(PC, LogPGUI, Log, TEXT("%s - PlayerController=%s - Skipping win sound as player is a spectator only : %s"),
			*GetName(), *PC->GetName(), *LoggingUtils::GetName<APlayerState>(MyPlayerState));
		return;
	}

	const auto Index = Players.IndexOfByKey(MyPlayerState);
	if(Index == INDEX_NONE)
	{
		UE_VLOG_UELOG(PC, LogPGUI, Warning, TEXT("%s - PlayerController=%s - Could not find player %s in sorted scores list : %s"),
			*GetName(), *PC->GetName(), *LoggingUtils::GetName<APlayerState>(MyPlayerState), *PG::ToStringObjectElements(Players));
		return;
	}

	UE_VLOG_UELOG(PC, LogPGUI, Log, TEXT("%s - PlayerController=%s - Found player %s at index %d in sorted scores list : %s"),
		*GetName(), *PC->GetName(), *LoggingUtils::GetName<APlayerState>(MyPlayerState), Index, *PG::ToStringObjectElements(Players));

	// Best score or tie
	checkf(Index >= 0 && Index < PlayerRanks.Num(), TEXT("Index=%d; PlayerRanks.Num()=%d"), Index, PlayerRanks.Num());
	const auto Rank = PlayerRanks[Index];

	if (Rank == 1)
	{
		PlaySound2D(WinSfx);
	}
	else
	{
		PlaySound2D(LoseSfx);
	}
}

void APGHUD::Init()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: Init"), *GetName());

	if (auto World = GetWorld(); ensure(World))
	{
		if (auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>(); ensure(GolfGameState))
		{
			GolfGameState->OnScoresSynced.AddUObject(this, &ThisClass::OnScoresSynced);
			GolfGameState->OnPlayerShotsUpdated.AddUObject(this, &ThisClass::OnCurrentHoleScoreUpdate);
			GolfGameState->OnPlayersChanged.AddUObject(this, &ThisClass::OnPlayersChanged);
			GolfGameState->OnPlayerScored.AddUObject(this, &ThisClass::OnPlayerGameStateSetScored);

			CheckForInitialDeferredState(*GolfGameState);
		}

		if (auto GolfEventsSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventsSubsystem))
		{
			GolfEventsSubsystem->OnPaperGolfPawnScored.AddUniqueDynamic(this, &ThisClass::OnPlayerScored);
			GolfEventsSubsystem->OnPaperGolfStartHole.AddUniqueDynamic(this, &ThisClass::OnStartHole);
			GolfEventsSubsystem->OnPaperGolfCourseComplete.AddUniqueDynamic(this, &ThisClass::OnCourseComplete);
			GolfEventsSubsystem->OnPaperGolfNextHole.AddUniqueDynamic(this, &ThisClass::OnHoleComplete);
		}
	}
}

#pragma region Event Handlers for HUD State

void APGHUD::OnScoresSynced(APaperGolfGameStateBase& GameState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnScoresSynced"), *GetName());

	bScoresSynced = bScoresEverSynced = true;

	if (!ShouldShowAnyScores())
	{
		return;
	}

	CheckShowScoresOrFinalResults(GameState);

	PlayCourseResultsSoundIfApplicable();
}

void APGHUD::CheckShowScoresOrFinalResults(const APaperGolfGameStateBase& GameState)
{
	if (!CheckShowFinalResults(&GameState))
	{
		const auto& GolfPlayerScores = GameState.GetSortedPlayerStatesByScore();
		ShowScoresHUD(GolfPlayerScores);
	}
}

void APGHUD::OnCurrentHoleScoreUpdate(APaperGolfGameStateBase& GameState, const AGolfPlayerState& PlayerState)
{
	// Don't update the scores if a 0 comes in as this would mean that the next hole is starting but we haven't received the event yet
	// We dismiss the current scores HUD on next hole event
	// We actually will not show the shots until the first second shot comes in
	if (PlayerState.GetShots() == 0)
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnCurrentHoleScoreUpdate: PlayerState=%s - Player has 0 shots"),
			*GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState));
		return;
	}

	// This happens when the hole score update comes in due to players scoring - See APaperGolfGameStateBase
	if (bHoleComplete)
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnCurrentHoleScoreUpdate: Hole already complete - PlayerState=%s"),
			*GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState));
		return;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnCurrentHoleScoreUpdate: PlayerState=%s - Shots=%d"),
		*GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState), PlayerState.GetShots());

	bShotUpdatesReceived = true;

	CheckNotifyHoleShotsUpdate(GameState);
}

void APGHUD::OnPlayersChanged(APaperGolfGameStateBase& GameState, const AGolfPlayerState& PlayerState, bool bPlayerAdded)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnPlayersChanged: PlayerState=%s; bPlayerAdded=%s"),
		*GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState), LoggingUtils::GetBoolString(bPlayerAdded));

	if (bPlayerAdded)
	{
		CheckExecuteDeferredSpectatorAction(PlayerState);
	}

	// hide player scores by default and then invoke logic to check if we should show it
	HideCurrentHoleScoresHUD();
	CheckNotifyHoleShotsUpdate(GameState);

	// Update standings
	if (bScoresEverSynced)
	{
		CheckShowScoresOrFinalResults(GameState);
	}
}

void APGHUD::OnPlayerGameStateSetScored(APaperGolfGameStateBase& GameState, const AGolfPlayerState& PlayerState)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnPlayerGameStateSetScored: PlayerState=%s"),
		*GetName(), *LoggingUtils::GetName<APlayerState>(PlayerState));

	// Need to update the player shots as an "F" is displayed next to the current shots if the player has scored
	OnCurrentHoleScoreUpdate(GameState, PlayerState);
}

void APGHUD::CheckNotifyHoleShotsUpdate(const APaperGolfGameStateBase& GameState)
{
	if (!bShotUpdatesReceived)
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckNotifyHoleShotsUpdate: bShotUpdatesReceived=%s - FALSE - No shots received yet"),
			*GetName(), LoggingUtils::GetBoolString(bShotUpdatesReceived));
		return;
	}

	auto GolfPlayerScores = GameState.GetSortedPlayerStatesByCurrentHoleScore();

	// Don't show if only 1 player
	if (GolfPlayerScores.Num() < 2)
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckNotifyHoleShotsUpdate - %s - Not enough players to show scores"),
			*GetName(), *LoggingUtils::GetName<APlayerState>(GolfPlayerScores[0]));
		HideCurrentHoleScoresHUD();
		return;
	}

	// Start showing the next shot when the turn activates, not after the actual shot is recorded to be consistent with the golf strokes HUD
	const bool bAllPlayersOneShot = !GolfPlayerScores.ContainsByPredicate([](const AGolfPlayerState* Player) { return !Player || Player->GetShots() == 0; }) &&
		GolfPlayerScores.ContainsByPredicate([](const AGolfPlayerState* Player) { return Player->GetShotsIncludingCurrent() > 1; });

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckNotifyHoleShotsUpdate - %s - Based on all player conditions"),
		*GetName(), LoggingUtils::GetBoolString(bAllPlayersOneShot));

	if (bAllPlayersOneShot)
	{
		if (bHideActivePlayerHoleScore && ActivePlayer)
		{
			// remove active turn player since it shows the stroke on the HUD
			GolfPlayerScores.Remove(const_cast<AGolfPlayerState*>(ActivePlayer->PlayerState.Get()));
		}

		ShowCurrentHoleScoresHUD(GolfPlayerScores);
	}
	else
	{
		HideCurrentHoleScoresHUD();
	}
}

bool APGHUD::CheckShowFinalResults(const APaperGolfGameStateBase* GameState)
{
	if (!ensure(GameState))
	{
		UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Error, TEXT("%s: CheckShowFinalResults - FALSE - GameState is NULL"), *GetName());
		return false;
	}

	const bool bShowFinalResults = FinalResultsAreDetermined();

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: CheckShowFinalResults - %s"),
		*GetName(), LoggingUtils::GetBoolString(bShowFinalResults));

	if (!bShowFinalResults)
	{
		return false;
	}

	RemoveActiveMessageWidget();
	HideCurrentHoleScoresHUD();
	HideScoresHUD();
	ShowFinalResultsHUD(GameState->GetSortedPlayerStatesByScore());

	return true;
}

void APGHUD::OnPlayerScored(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnPaperGolfPawnScored: %s"),
		*GetName(), *LoggingUtils::GetName(PaperGolfPawn));

	PlaySound2D(ScoredSfx);
}

void APGHUD::OnStartHole(int32 HoleNumber)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnStartHole: HoleNumber=%d"), *GetName(), HoleNumber);

	bScoresSynced = bHoleComplete = bCourseComplete = false;
}

void APGHUD::OnCourseComplete()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnCourseComplete"), *GetName());

	bCourseComplete = true;
	ActivePlayer.Reset();
	bShotUpdatesReceived = false;

	if (!ShouldShowAnyScores())
	{
		return;
	}

	if (!CheckShowFinalResults(GetGameState()))
	{
		HideCurrentHoleScoresHUD();
	}

	PlayCourseResultsSoundIfApplicable();
}

void APGHUD::OnHoleComplete()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnHoleComplete"), *GetName());

	ActivePlayer.Reset();
	bShotUpdatesReceived = false;
	bHoleComplete = true;

	HideCurrentHoleScoresHUD();
}

#pragma endregion Event Handlers for HUD State

APGHUD::FActivePlayer::FActivePlayer(const AGolfPlayerState& InPlayerState)
{
	PlayerState = &InPlayerState;
	Id = InPlayerState.GetPlayerId();
}
