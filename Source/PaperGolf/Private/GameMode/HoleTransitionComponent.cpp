// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "GameMode/HoleTransitionComponent.h"

// TODO: May not need the game mode reference here if using events
#include "GameMode/PaperGolfGameModeBase.h"

#include "Logging/LoggingUtils.h"
#include "Utils/ArrayUtils.h"
#include "Utils/StringUtils.h"
#include "VisualLogger/VisualLogger.h"
#include "PaperGolfLogging.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"

#include "Kismet/GameplayStatics.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Interfaces/GolfController.h"

#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"


UHoleTransitionComponent::UHoleTransitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AActor* UHoleTransitionComponent::ChoosePlayerStart(AController* Player)
{
	// TODO: Use GolfPlayerStart and follow implementations in LyraPlayerSpawningManagerComponent.cpp and LyraGameMode for examples of how player starts customized
	// Also can see ShooterGameMode as well

	return nullptr;
}

void UHoleTransitionComponent::OnNextHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: OnNextHole"), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	auto Delegate = FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (auto World = GetWorld(); ensure(World))
			{
				// TODO: broadcast start hole event once find next hole
				World->ServerTravel("?Restart");
			}
		});

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, Delegate, NextHoleDelay, false);
}

void UHoleTransitionComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPaperGolfGame, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	if (auto GolfEventSubsystem = World->GetSubsystem<UGolfEventsSubsystem>(); ensure(GolfEventSubsystem))
	{
		GolfEventSubsystem->OnPaperGolfNextHole.AddDynamic(this, &ThisClass::OnNextHole);
	}
}
