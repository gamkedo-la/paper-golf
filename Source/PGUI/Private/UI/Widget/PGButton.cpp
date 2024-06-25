// Copyright Game Salutes. All Rights Reserved.


#include "UI/Widget/PGButton.h"

#include "Logging/LoggingUtils.h"
#include "PGUILogging.h"

#include "Subsystems/RealtimeTimerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PGButton)

void UPGButton::PostInitProperties()
{
	Super::PostInitProperties();

	// Make sure world is valid and it's not a PIE world
	auto World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	BindEvents();
}

void UPGButton::BeginDestroy()
{
	Super::BeginDestroy();

	UnregisterTimer();
}

void UPGButton::BindEvents()
{
	if (HoverMaxScale > 1)
	{
		OnHovered.AddUniqueDynamic(this, &ThisClass::DoHover);
	}
}

void UPGButton::DoHover()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	HoverStartTime = World->GetRealTimeSeconds() - FMath::Max(0.0f, HoverDelayTime);

	if (auto TimerSystem = World->GetSubsystem<URealtimeTimerSubsystem>(); ensureMsgf(IsValid(TimerSystem), TEXT("URealtimeTimerSubsystem is NULL")))
	{
		TimerSystem->RealTimeTimerDelegate.AddUObject(this, &ThisClass::TickHover);
	}
}

void UPGButton::TickHover(float DeltaTime)
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto TimeSeconds = World->GetRealTimeSeconds();
	const auto TimeElapsed = TimeSeconds - HoverStartTime;

	if (TimeElapsed >= HoverScaleTime)
	{
		SetRenderScale({ 1.0f, 1.0f });
		UnregisterTimer();

		return;
	}

	// flip scaling midway through

	auto Alpha = TimeElapsed / (HoverScaleTime * 0.5);
	if (Alpha > 1)
	{
		Alpha = 1 - FMath::Fractional(Alpha);
	}

	const auto Scale = FMath::InterpEaseInOut(1.0f, HoverMaxScale, Alpha, HoverEaseFactor);
	SetRenderScale({ Scale, Scale });
}

void UPGButton::UnregisterTimer()
{
	if (!HoverHandle.IsValid())
	{
		return;
	}

	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	if (auto TimerSystem = World->GetSubsystem<URealtimeTimerSubsystem>(); IsValid(TimerSystem))
	{
		TimerSystem->RealTimeTimerDelegate.Remove(HoverHandle);
	}
}
