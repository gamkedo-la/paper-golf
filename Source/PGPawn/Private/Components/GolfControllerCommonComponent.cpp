// Copyright Game Salutes. All Rights Reserved.


#include "Components/GolfControllerCommonComponent.h"

#include "Interfaces/GolfController.h"
#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "Kismet/GameplayStatics.h"

#include "GameFramework/SpectatorPawn.h"

#include "PGPawnLogging.h"
#include "Logging/LoggingUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Utils/StringUtils.h"

#include "PaperGolfTypes.h"


#include "State/GolfPlayerState.h"
#include "State/PaperGolfGameStateBase.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include <limits>
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfControllerCommonComponent)

UGolfControllerCommonComponent::UGolfControllerCommonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGolfControllerCommonComponent::BeginPlay()
{
	Super::BeginPlay();

	GolfController = GetOwner();
	if(!ensureAlwaysMsgf(GolfController, TEXT("%s-%s owner is not a IGolfController"), *GetName(), *LoggingUtils::GetName(GetOwner())))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s owner is not a IGolfController"), *GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
}

void UGolfControllerCommonComponent::Init()
{
}

void UGolfControllerCommonComponent::DeferredInit()
{
}

void UGolfControllerCommonComponent::OnFellThroughFloor(APaperGolfPawn* InPaperGolfPawn)
{
}

void UGolfControllerCommonComponent::ResetRotation()
{
}

void UGolfControllerCommonComponent::ResetShot()
{
}

bool UGolfControllerCommonComponent::IsReadyForShot() const
{
	return false;
}

bool UGolfControllerCommonComponent::HandleOutOfBounds()
{
	return false;
}

bool UGolfControllerCommonComponent::HasScored() const
{
	return false;
}

void UGolfControllerCommonComponent::MarkScored()
{
}

void UGolfControllerCommonComponent::SnapToGround()
{
}

void UGolfControllerCommonComponent::AddPaperGolfPawnRelativeRotation(const FRotator& DeltaRotation)
{
}

void UGolfControllerCommonComponent::SetShotType(EShotType InShotType)
{
}

void UGolfControllerCommonComponent::ToggleShotType()
{
}

void UGolfControllerCommonComponent::DestroyPawn()
{
}

void UGolfControllerCommonComponent::DetermineShotType(const AActor& GolfHole)
{
}

void UGolfControllerCommonComponent::AddToShotHistory(APaperGolfPawn* PaperGolfPawn)
{
	if (!PaperGolfPawn)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Warning, TEXT("%s: AddToShotHistory - PaperGolfPawn is NULL"), *GetName());
		return;
	}

	const auto& ActorLocation = PaperGolfPawn->GetActorLocation();

	const auto Size = ShotHistory.Num();

	[[maybe_unused]] const auto Index = ShotHistory.AddUnique({ ActorLocation });

	if (ShotHistory.Num() > Size)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s: AddToShotHistory - PaperGolfPawn=%s - New shot added to history: %s; Count=%d"),
			*GetName(),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			ShotHistory.Num());
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s: AddToShotHistory - PaperGolfPawn=%s - Duplicate shot not added to history: %s; ExistingIndex=%d; Count=%d"),
			*GetName(),
			*PaperGolfPawn->GetName(),
			*ActorLocation.ToCompactString(),
			Index,
			ShotHistory.Num());
	}
}

bool UGolfControllerCommonComponent::IsFlickedAtRest() const
{
	return false;
}

void UGolfControllerCommonComponent::ResetShotAfterOutOfBounds()
{
}

void UGolfControllerCommonComponent::RegisterShotFinishedTimer()
{
}

void UGolfControllerCommonComponent::UnregisterShotFinishedTimer()
{
}

void UGolfControllerCommonComponent::RegisterGolfSubsystemEvents()
{
}

void UGolfControllerCommonComponent::HandleFallThroughFloor()
{
}

bool UGolfControllerCommonComponent::IsReadyForNextShot() const
{
	return false;
}

void UGolfControllerCommonComponent::SetupNextShot(bool bSetCanFlick)
{
}

void UGolfControllerCommonComponent::SetPositionTo(const FVector& Position, const TOptional<FRotator>& OptionalRotation)
{
}

void UGolfControllerCommonComponent::CheckForNextShot()
{
}

bool UGolfControllerCommonComponent::CanFlick() const
{
	return false;
}
