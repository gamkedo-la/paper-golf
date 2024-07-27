// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "UI/PGHUD.h"

#include "Logging/LoggingUtils.h"
#include "PGUILogging.h"

#include "Blueprint/UserWidget.h"

#include "Runtime/CoreUObject/Public/UObject/SoftObjectPtr.h"
#include "Runtime/Engine/Classes/Engine/StreamableManager.h"

#include "Engine/AssetManager.h"

#include "Subsystems/GolfEventsSubsystem.h"
#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Pawn/PaperGolfPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGHUD)

void APGHUD::ShowHUD()
{
	Super::ShowHUD();

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: ShowHUD: %s"), *GetName(), LoggingUtils::GetBoolString(bShowHUD));

	OnToggleHUDVisibility(bShowHUD);
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
	case EMessageWidgetType::OutOfBounds:
		DisplayMessageWidgetByClass(OutOfBoundsWidgetClass);
		break;
	case EMessageWidgetType::HoleFinished:
		DisplayMessageWidgetByClass(HoleFinishedWidgetClass);
		break;
	case EMessageWidgetType::Tutorial:
		DisplayMessageWidgetByClass(TutorialWidgetClass);
		break;
	default:
		checkNoEntry();
	}
}

void APGHUD::RemoveActiveMessageWidget()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: RemoveActiveMessageWidget: ActiveMessageWidget=%s"),
		*GetName(), *LoggingUtils::GetName(ActiveMessageWidget));
	if(!IsValid(ActiveMessageWidget))
	{
		return;
	}

	ActiveMessageWidget->RemoveFromParent();
	ActiveMessageWidget = nullptr;
}

void APGHUD::SpectatePlayer_Implementation(APaperGolfPawn* PlayerPawn)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log,
		TEXT("%s: SpectatePlayer: PlayerPawn=%s"), *GetName(), *LoggingUtils::GetName(PlayerPawn));
}

void APGHUD::BeginTurn_Implementation()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginTurn"), *GetName());
}

void APGHUD::BeginPlay()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	Init();
}

void APGHUD::DisplayMessageWidgetByClass(const TSoftClassPtr<UUserWidget>& WidgetClass)
{
	if (!ensure(!WidgetClass.IsNull()))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: DisplayMessageWidgetByClass: %s"), *GetName(), *LoggingUtils::GetName(WidgetClass));

	ActiveMessageWidgetClass = WidgetClass;

	// Must use this versino and not just create an instance of FStreamableManager as then the loading doesn't work when the callback fires!
	auto& StreamableManager = UAssetManager::Get().GetStreamableManager();

	// Request to load the asset asynchronously
	StreamableManager.RequestAsyncLoad(WidgetClass.ToSoftObjectPath(), [this, WeakThis = TWeakObjectPtr<APGHUD>(this), WidgetClass]()
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

		// This lambda is executed once the asset has been loaded
		UClass* LoadedClass = WidgetClass.Get();
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

		RemoveActiveMessageWidget();

		ActiveMessageWidget = NewWidget;
		ActiveMessageWidget->AddToViewport();
	});
}

void APGHUD::Init()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: Init"), *GetName());

	if (auto World = GetWorld(); ensure(World))
	{
		if (auto GolfGameState = World->GetGameState<APaperGolfGameStateBase>(); ensure(GolfGameState))
		{
			GolfGameState->OnScoresSynced.AddUObject(this, &ThisClass::OnScoresSynced);
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

	// TODO: Call function  to update the scores and show them for the first time if they weren't showing before
	// 
	// TODO: OnStartHole for HoleNumber 1 doesn't get called because the event happens before we can subscribe
	// We could just do what we normally would on BeginPlay to cover that case
	// 
	// we can listen for start hole, next hole, course complete from here to show the appropriate widgets
	// For example, hole finishes - we show the score card for players and current rankings and then also update the HUD 
	// - which is actually equivalent to what we are doing in this function
	// We may just want to use the combo of scores synced and then course complete, next hole (previous hole finished) to determine how to display the results
	// Start Hole could be used to trigger the hole flyby and maybe show the updated player rankings from last hole

	const auto& GolfPlayerScores = GameState.GetSortedPlayerStatesByScore();

	ShowScoresHUD(GolfPlayerScores);
}

void APGHUD::OnPlayerScored(APaperGolfPawn* PaperGolfPawn)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnPaperGolfPawnScored: %s"),
		*GetName(), *LoggingUtils::GetName(PaperGolfPawn));
}

void APGHUD::OnStartHole(int32 HoleNumber)
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnStartHole: HoleNumber=%d"), *GetName(), HoleNumber);
}

void APGHUD::OnCourseComplete()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnCourseComplete"), *GetName());
}

void APGHUD::OnHoleComplete()
{
	UE_VLOG_UELOG(GetOwningPlayerController(), LogPGUI, Log, TEXT("%s: OnHoleComplete"), *GetName());
}

#pragma endregion Event Handlers for HUD State
