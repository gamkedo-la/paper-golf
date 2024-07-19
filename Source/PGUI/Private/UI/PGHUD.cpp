// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "UI/PGHUD.h"

#include "Logging/LoggingUtils.h"
#include "PGUILogging.h"

#include "Blueprint/UserWidget.h"

#include "Runtime/CoreUObject/Public/UObject/SoftObjectPtr.h"
#include "Runtime/Engine/Classes/Engine/StreamableManager.h"

#include "Engine/AssetManager.h"

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
