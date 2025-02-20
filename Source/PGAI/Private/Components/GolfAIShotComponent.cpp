// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/GolfAIShotComponent.h"


#include "Pawn/PaperGolfPawn.h"

#include "Library/PaperGolfPawnUtilities.h"

#include "State/GolfPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"
#include "Utils/CollisionUtils.h"

#include "Utils/GolfAIShotCalculationUtils.h"

#include "Utils/ArrayUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/PGMathUtils.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/CurveTable.h"
#include "Curves/RealCurve.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

#include "AIStrategy/AIPerformanceStrategy.h"

#include "Obstacles/PenaltyHazard.h"

#include "PGTags.h"

#include <array>
#include <iterator>
#include <algorithm>
#include <limits>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIShotComponent)

namespace
{
	constexpr const std::array ShotPitchAngles = { 45.0f, 60.0f, 75.0f, 30.0f, 0.0f, -15.0f };

	// Gets the power fraction for a given delta distance in meters from the hole
	const FName DeltaDistanceMetersVsPowerFraction = TEXT("DeltaDistanceM_Power");

	// Gets the pitch angle to hit the shot vs the power fraction
	const FName PowerFractionVsPitchAngle = TEXT("Power_Angle");

	// Get the Z Offset to use for a given delta distnace in meters from the hole
	const FName DeltaDistanceMetersVsZOffset = TEXT("DeltaDistanceM_ZOffset");

	// Gets the power multiplier for a new angle after avoidance calculation by taking the new abs delta diff from 45 degrees
	const FName AngleDeviationVsPowerMultiplier = TEXT("AngleDelta_Power");

	FRealCurve* FindCurveForKey(UCurveTable* CurveTable, const FName& Key);
	
	constexpr float DefaultPitchAngle = 45.0f;

	constexpr double DistSqHitThreshold = FMath::Square(10);

	FVector GetVelocityAtHitPoint(const FPredictProjectilePathResult& PathResult);
}

UGolfAIShotComponent::UGolfAIShotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FAIShotSetupResult UGolfAIShotComponent::SetupShot(FAIShotContext&& InShotContext)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UGolfAIShotComponent::SetupShot");

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: SetupShot - ShotContext=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *InShotContext.ToString());

	ShotContext = std::move(InShotContext);
	CurrentFocusActorFailures = 0;
	bCurrentFocusActorLandedInHazard = false;

	check(ShotContext.PlayerPawn);
	FocusActor = ShotContext.PlayerPawn->GetFocusActor();
	InitialFocusYaw = ShotContext.PlayerPawn->GetActorRotation().Yaw;

	auto ShotParams = CalculateShotParams();

	if (!ShotParams)
	{
		ShotParams = CalculateDefaultShotParams();
	}

	check(ShotParams);

	HoleShotResults.Add(FShotResult
	{
		.ShotSetupParams = *ShotParams
	});

	ShotContext = {};

	return ShotParams->ShotSetupResult;
}

void UGolfAIShotComponent::StartHole()
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: StartHole"), *LoggingUtils::GetName(GetOwner()), *GetName());

	// TODO: This is where we can reset the strategy for the hole based on the AI's score relative to players in the game
	// A good spot to reshuffle the shot configs, mixing in perfect and error shots

	ResetHoleData();
}

void UGolfAIShotComponent::Reset()
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: Reset"), *LoggingUtils::GetName(GetOwner()), *GetName());

	ResetHoleData();
}

void UGolfAIShotComponent::ResetHoleData()
{
	FocusActor = nullptr;
	InitialFocusYaw = 0;
	HoleShotResults.Reset();
	CurrentFocusActorFailures = 0;
	bCurrentFocusActorLandedInHazard = false;
	DistanceToHole = -1;
}

void UGolfAIShotComponent::OnHazard(EHazardType HazardType)
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: HandleHazard - HazardType=%s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HazardType));

	if (ensure(!HoleShotResults.IsEmpty()))
	{
		HoleShotResults.Last().HitHazard = HazardType;
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: HandleHazard -- HazardType=%s - HoleShotResults is empty"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HazardType));
	}
}

void UGolfAIShotComponent::OnShotFinished(const APaperGolfPawn& Pawn)
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: OnShotFinished: Pawn=%s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(Pawn));

	if (ensure(!HoleShotResults.IsEmpty()))
	{
		HoleShotResults.Last().EndPosition = Pawn.GetPaperGolfPosition();
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: OnShotFinished - HoleShotResults is empty"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}
}

void UGolfAIShotComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::BeginPlay();

	ValidateAndLoadConfig();
}

TOptional<UGolfAIShotComponent::FShotSetupParams> UGolfAIShotComponent::CalculateShotParams()
{
	const auto& FocusActorScores = ShotContext.FocusActorScores;
	const bool bOnlySingleFocusAvailable = FocusActorScores.Num() <= 1;

	// If only a single focus actor available, then compute the number of consecutive failures for pitch angle adjustment
	const int32 FirstResultFailureTolerance = bOnlySingleFocusAvailable ? ShotPitchAngles.size() : ConsecutiveFailureCurrentFocusLimit;
	const bool bFirstResultViable = IsFocusActorViableBasedOnShotHistory(FocusActor, FirstResultFailureTolerance, &CurrentFocusActorFailures, &bCurrentFocusActorLandedInHazard);

	const auto FirstResult = CalculateShotParamsForCurrentFocusActor();

	if (bOnlySingleFocusAvailable)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotParams - Insufficient alternative focii to check for hazards - FocusActorCount=%d, using first result=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), FocusActorScores.Num(), *PG::StringUtils::ToString(FirstResult));
		return FirstResult;
	}

	const auto FirstFocusActor = FocusActor;

	if (FirstResult && bFirstResultViable && !ShotWillEndUpInHazard(FirstResult->ShotSetupResult))
	{
		return FirstResult;
	}

	for (const auto& FocusActorScore : FocusActorScores)
	{
		if (FocusActorScore.FocusActor == FirstFocusActor)
		{
			continue;
		}

		// Need to filter the focus actors to the ones that haven't resulted in multiple failures
		int32 FocusActorFailures{};
		if (!IsFocusActorViableBasedOnShotHistory(FocusActorScore.FocusActor, ConsecutiveFailureCurrentFocusLimit, &FocusActorFailures, &bCurrentFocusActorLandedInHazard))
		{
			continue;
		}

		FocusActor = FocusActorScore.FocusActor;
		const auto Result = CalculateShotParamsForCurrentFocusActor();

		if (Result && !ShotWillEndUpInHazard(Result->ShotSetupResult))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotParams - Using FocusActor=%s; Result=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FocusActor), *PG::StringUtils::ToString(Result));
			CurrentFocusActorFailures = FocusActorFailures;
			return Result;
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotParams - No viable alternative focus actor found, using first result - FocusActor=%s; Result=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FirstFocusActor), *PG::StringUtils::ToString(FirstResult));

	FocusActor = FirstFocusActor;

	return FirstResult;
}

bool UGolfAIShotComponent::ShotWillEndUpInHazard(const FAIShotSetupResult& ShotSetupResult) const
{
	const auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	// Must account for additional rotation on top of the current actor rotation
	const FRotator AdditionalRotation = { ShotSetupResult.ShotPitch, PG::MathUtils::ClampDeltaYaw(PlayerPawn->GetRotationYawToFocusActor(FocusActor) + ShotSetupResult.ShotYaw - InitialFocusYaw), 0 };

	FFlickPredictParams FlickPredictParams
	{
		.AdditionalWorldRotation = AdditionalRotation,
	};

	FPredictProjectilePathResult PathResult;

	if (!PlayerPawn->PredictFlick(ShotSetupResult.FlickParams, FlickPredictParams, PathResult))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: ShotWillEndUpInHazard - FALSE - PredictFlick did not hit anything"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return false;
	}

	const FVector& HitLocation = PathResult.HitResult.ImpactPoint;

	// First do a trace to the initial hit result to see if its a hazard in that location and if so what type to see if we need to check for bounce
	auto PenaltyHazardOptional = TraceToHazard(HitLocation);
	if (PenaltyHazardOptional && !PenaltyHazardOptional.GetValue()->IsPenaltyOnImpact())
	{
		// extend the hit location to account for bounce
		const auto BounceLocation = GetBounceLocation(PathResult);

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: ShotWillEndUpInHazard - Detected impact hit at %s but %s hazard allows for bounce - Re-testing at final location %s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			*HitLocation.ToCompactString(), *LoggingUtils::GetName(PenaltyHazardOptional.GetValue()->GetHazardType()), *BounceLocation.ToCompactString());

		UE_VLOG_LOCATION(GetOwner(), LogPGAI, Log, BounceLocation, 10.0f, FColor::Yellow, TEXT("Bounce"));

		PenaltyHazardOptional = TraceToHazard(BounceLocation);
	}

	const bool bHazard = PenaltyHazardOptional.IsSet();

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: ShotWillEndUpInHazard - %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LoggingUtils::GetBoolString(bHazard));

	return bHazard;
}

float UGolfAIShotComponent::GetPowerMultiplierFromAngleDeviation(float YawDelta, float OriginalPitchAngleDegrees, float NewPitchAngleDegrees) const
{
	if (FMath::Abs(YawDelta) > MaxYawPitchAnglePowerMultiplierApplication)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: GetPowerMultiplierFromAngleDeviation - Abs(YawDelta)=%.1f > %.1f; returning 1.0"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), YawDelta, MaxYawPitchAnglePowerMultiplierApplication);
		return 1.0f;
	}

	const auto Curve = FindCurveForKey(AIConfigCurveTable, AngleDeviationVsPowerMultiplier);
	if (!Curve)
	{
		return 1.0f;
	}

	// The deviation from 45 is what will affect the total distance so must compare the relative deviation difference between the original and new angles
	const auto AngleDeviation = FMath::Abs(NewPitchAngleDegrees - 45) - FMath::Abs(OriginalPitchAngleDegrees - 45);
	const auto PowerMultiplier = Curve->Eval(AngleDeviation);

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: GetPowerMultiplierFromAngleDeviation - YawDelta=%1.f; OriginalPitchAngleDegrees=%.1f; NewPitchAngleDegrees=%.1f; AngleDeviation=%.1f; PowerMultiplier=%.2f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), YawDelta, OriginalPitchAngleDegrees, NewPitchAngleDegrees, AngleDeviation, PowerMultiplier);

	return PowerMultiplier;
}

FVector UGolfAIShotComponent::GetBounceLocation(const FPredictProjectilePathResult& PathResult) const
{
	const FVector VelocityAtHitPoint = GetVelocityAtHitPoint(PathResult);
	const FVector VelocityDirection = VelocityAtHitPoint.GetSafeNormal();
	const auto Speed = VelocityAtHitPoint.Size();
	// reduce speed by elasticity -> KE = 1/2mv^2 -> v reduced by factor of sqrt(elasticity)
	const auto Restitution = GetHitRestitution(PathResult.HitResult);
	const auto BounceSpeed = Speed * FMath::Sqrt(Restitution);

	const FVector BounceDirection = FMath::GetReflectionVector(VelocityDirection, PathResult.HitResult.ImpactNormal);
	const auto& HitLocation = PathResult.HitResult.ImpactPoint;

	const auto BounceLocation = HitLocation + BounceDirection * BounceSpeed * HazardPredictBounceTime;

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
		TEXT("%s-%s: BounceLocation - Restitution=%f; VelocityAtHitPoint=%s; Speed=%f; BounceDirection=%s; HitLocation=%s; BounceLocation=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		Restitution, *VelocityAtHitPoint.ToCompactString(), Speed, *BounceDirection.ToCompactString(), *HitLocation.ToCompactString(),
		*BounceLocation.ToCompactString());

	return BounceLocation;
}

TOptional<const IPenaltyHazard*> UGolfAIShotComponent::TraceToHazard(const FVector& Location) const
{
	auto World = GetWorld();
	if (!ensure(World))
	{
		return {};
	}

	FHitResult HazardHitResult;
	const auto TraceOffset = FVector::ZAxisVector * HazardTraceDistance * 0.5f;

	const auto HazardTraceStart = Location + TraceOffset;
	const auto HazardTraceEnd = Location - TraceOffset;

	const bool bTraceResult = World->LineTraceSingleByObjectType(HazardHitResult, HazardTraceStart, HazardTraceEnd, PG::CollisionObjectType::Hazard);
	if (bTraceResult)
	{
		// Cast the actor to IPenaltyHazard
		const auto HazardActor = HazardHitResult.GetActor();
		const auto PenaltyHazard = Cast<const IPenaltyHazard>(HazardActor);
		if (!ensure(HazardActor && PenaltyHazard))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: TraceToHazard - TRUE - HazardActor=%s was not a penalty hazard!"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HazardActor));
			return {};
		}

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: TraceToHazard - TRUE - Found %s Hazard: %s at %s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(PenaltyHazard->GetHazardType()), *LoggingUtils::GetName(HazardActor), *Location.ToCompactString());

		UE_VLOG_ARROW(GetOwner(), LogPGAI, Log, HazardTraceStart, HazardTraceEnd, FColor::Red, TEXT("%s Hazard: %s"),
			*LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(PenaltyHazard->GetHazardType()), *LoggingUtils::GetName(HazardActor));

		return PenaltyHazard;
	}

	UE_VLOG_ARROW(GetOwner(), LogPGAI, Verbose, HazardTraceStart, HazardTraceEnd, FColor::Yellow, TEXT("No Hazard"));

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: TraceToHazard - FALSE - Did not find a hazard at %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *Location.ToCompactString());

	return {};
}

float UGolfAIShotComponent::GetHitRestitution(const FHitResult& HitResult) const
{
	// Ideally we would use PathResult.HitResult.PhysMaterial->Restitution; however, the path prediction hit result is not returned with physical material but we can do a quick trace to get it
	if (auto PhysicalMaterial = HitResult.PhysMaterial.Get(); PhysicalMaterial)
	{
		return PhysicalMaterial->Restitution;
	}

	// do a trace to get the physical material since the original hit did not have it
	auto World = GetWorld();
	if (!ensure(World))
	{
		return 0.0f;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ShotContext.PlayerPawn);
	Params.bReturnPhysicalMaterial = true;

	FHitResult TraceHitResult;

	if (!World->LineTraceSingleByChannel(TraceHitResult, HitResult.ImpactPoint + HitResult.ImpactNormal * 10, HitResult.ImpactPoint - HitResult.ImpactNormal * 10, PG::CollisionChannel::FlickTraceType))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: GetHitRestitution - Unable to determine physical material from re-trace; ImpactPoint=%s; ImpactNormal=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *HitResult.ImpactPoint.ToCompactString(), *HitResult.ImpactNormal.ToCompactString());
		return 0.0f;
	}

	if (auto PhysicalMaterial = TraceHitResult.PhysMaterial.Get(); PhysicalMaterial)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
			TEXT("%s-%s: GetHitRestitution - %f - PhysicalMaterial=%s; Actor=%s; Component=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PhysicalMaterial->Restitution, *PhysicalMaterial->GetName(),
			*LoggingUtils::GetName(TraceHitResult.GetActor()), *LoggingUtils::GetName(TraceHitResult.GetComponent()));

		return PhysicalMaterial->Restitution;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Warning,
		TEXT("%s-%s: GetHitRestitution - %f - Re-trace hit result did not return physical material using default! OriginalImpactPoint=%s; OriginalImpactNormal=%s; TraceImpactPoint=%s; TraceImpactNormal=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), DefaultHitRestitution,
		*HitResult.ImpactPoint.ToCompactString(), *HitResult.ImpactNormal.ToCompactString(),
		*TraceHitResult.ImpactPoint.ToCompactString(), *TraceHitResult.ImpactNormal.ToCompactString());

	return DefaultHitRestitution;
}

TOptional<UGolfAIShotComponent::FShotSetupParams> UGolfAIShotComponent::CalculateShotParamsForCurrentFocusActor()
{
	const auto InitialShotParamsOptional = CalculateInitialShotParams();
	if (!InitialShotParamsOptional)
	{
		return {};
	}

	const auto& InitialShotParams = *InitialShotParamsOptional;

	auto FlickParams = InitialShotParams.ToFlickParams();

	// Calibrate the power fraction and ideal pitch based on the current distance to hole (ignoring obstacles)
	const auto ShotCalibrationResult = CalibrateShot(InitialShotParams.FlickLocation, InitialShotParams.PowerFraction);
	FlickParams.PowerFraction = ShotCalibrationResult.PowerFraction;
	FlickParams.LocalZOffset = ShotCalibrationResult.LocalZOffset;

	// calculate the pitch and yaw based on obstacles obstructing the shot
	const auto& ShotCalculationResult = CalculateAvoidanceShotFactors(
		InitialShotParams.FlickLocation, ShotCalibrationResult.Pitch, InitialShotParams.FlickMaxSpeed, FlickParams.PowerFraction);

	const auto AngleDeviationPowerMultiplier = GetPowerMultiplierFromAngleDeviation(ShotCalculationResult.Yaw, ShotCalibrationResult.Pitch, ShotCalculationResult.Pitch);
	// Decrease the power fraction based on avoidance results but make sure it doesn't go below the minimum power reduction factor
	// The min result is done through the AIPerformanceStrategy
	const auto InitialPowerFraction = FlickParams.PowerFraction *= FMath::Max(ShotCalculationResult.PowerMultiplier * AngleDeviationPowerMultiplier, MinRetryShotPowerReductionFactor);

	// Add error to power calculation and accuracy
	if (AIPerformanceStrategy)
	{
		const auto FlickError = AIPerformanceStrategy->CalculateShotError(FlickParams);
		FlickParams.PowerFraction = FlickError.PowerFraction;
		FlickParams.Accuracy = FlickError.Accuracy;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotParamsForCurrentFocusActor - ShotType=%s; InitialPower=%.2f; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f; ShotPitch=%.1f; ShotYaw=%.1f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FlickParams.ShotType),
		InitialPowerFraction,
		FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset,
		ShotCalculationResult.Pitch, ShotCalculationResult.Yaw
	);

	return FShotSetupParams
	{
		.FlickLocation = InitialShotParams.FlickLocation,
		.InitialPowerFraction = InitialPowerFraction,
		.ShotSetupResult = FAIShotSetupResult
		{
			.FlickParams = FlickParams,
			.FocusActor = FocusActor,
			.ShotPitch = ShotCalculationResult.Pitch,
			.ShotYaw = ShotCalculationResult.Yaw
		}
	};
}

bool UGolfAIShotComponent::IsFocusActorViableBasedOnShotHistory(const AActor* CurrentFocusActor, int32 MaxFailures, int32* OutNumFailures, bool* bOutCurrentFocusActorLandedInHazard) const
{
	if (!CurrentFocusActor)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Warning, TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory - FALSE - FocusActor is null"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return false;
	}

	if (HoleShotResults.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory - TRUE - HoleShotResults is empty"),
			*LoggingUtils::GetName(GetOwner()), *GetName());
		return true;
	}

	const auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	if (DistanceToHole < 0)
	{
		const auto GolfHole = ShotContext.GolfHole;
		check(GolfHole);

		DistanceToHole = PlayerPawn->GetDistanceTo(GolfHole);
	}

	if (DistanceToHole < MinDistanceSuccessTest)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory - TRUE - DistanceToHole=%.1fm is less than MinDistanceSuccessTest=%.1fm"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), DistanceToHole / 100, MinDistanceSuccessTest / 100);
		return true;
	}

	// Start at the end of the history and break if the focus actor changes once we find a match
	const AActor* LastShotResultFocus{};

	// memoize the distance to the focus actor
	auto DistanceToFocusSqFn = [&, Dist = -1.0f]() mutable
	{
		if (Dist >= 0)
		{
			return Dist;
		}
		return Dist = PlayerPawn->GetSquaredDistanceTo(CurrentFocusActor);
	};

	const auto& CurrentFlickLocation = PlayerPawn->GetActorLocation();
	const auto& FocusActorLocation = CurrentFocusActor->GetActorLocation();

	//const auto CurrentToFocusActor = FocusActorLocation - CurrentFlickLocation;

	int32 ConsecutiveShotCount = 0;
	bool bHitHazard = false;

	const auto SetOutFailures = [&]
	{
		if (OutNumFailures)
		{
			*OutNumFailures = ConsecutiveShotCount;
		}

		if (bCurrentFocusActorLandedInHazard)
		{
			*bOutCurrentFocusActorLandedInHazard = bHitHazard;
		}
	};

	for (int32 i = HoleShotResults.Num() - 1; i >= 0; --i)
	{
		const auto& ShotResult = HoleShotResults[i];
		const auto& ShotSetupResult = ShotResult.ShotSetupParams.ShotSetupResult;
		const auto HistoryFocusActor = ShotSetupResult.FocusActor;

		if (LastShotResultFocus == CurrentFocusActor && HistoryFocusActor != CurrentFocusActor)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - TRUE - FocusActor was not used recently. Looked at %d shots with that actor; %d/%d shots examined"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor),
				ConsecutiveShotCount, HoleShotResults.Num() - i, HoleShotResults.Num());
			SetOutFailures();
			return true;
		}

		if (HistoryFocusActor != CurrentFocusActor)
		{
			continue;
		}

		const auto& FlickParams = ShotSetupResult.FlickParams;

		// Determine if this shot should be ignored because shot error strategy influenced it too much to be a valid data point
		if (FMath::Abs(FlickParams.Accuracy) > MaxAccuracyDeviationRetry)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Ignoring shot %d because Abs Accuracy=%.2f > MaxAccuracyDeviationRetry=%.2f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), i + 1, FlickParams.Accuracy, MaxAccuracyDeviationRetry);
			continue;
		}

		if (FMath::Abs(ShotResult.ShotSetupParams.GetPowerDeviation()) > MaxPowerDeviationRetry)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Ignoring shot %d because Abs PowerDeviation=%.2f > MaxPowerDeviationRetry=%.2f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), i + 1, ShotResult.ShotSetupParams.GetPowerDeviation(), MaxPowerDeviationRetry);
			continue;
		}

		// Determine if this shot is even relevant based on position
		const auto& DistanceSqToShotResult = FVector::DistSquared(ShotResult.ShotSetupParams.FlickLocation, CurrentFlickLocation);
		if (DistanceSqToShotResult > FMath::Square(ShotHistoryRadiusRelevance))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Ignoring shot %d because DistanceSqToShotResult=%.1fm > ShotHistoryRadiusRelevance=%.1fm"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), i + 1, FMath::Sqrt(DistanceSqToShotResult) / 100, ShotHistoryRadiusRelevance / 100);
			continue;
		}

		// If we went into a hazard, automatically count it
		if (ShotResult.HitHazard)
		{
			++ConsecutiveShotCount;
			bHitHazard = true;

			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Shot %d was a hazard, retry %d/%d"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), i + 1, ConsecutiveShotCount, MaxFailures);

			if (ConsecutiveShotCount >= MaxFailures)
			{
				UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
					TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - FALSE - FocusActor has had %d consecutive failures"),
					*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), ConsecutiveShotCount);
				SetOutFailures();
				return false;
			}
			continue;
		}

		// Check based on distance result

		// See if this attempt actually should be considered a success because it made enough progress to the target
		const auto DistToFocusSq = DistanceToFocusSqFn();
		const auto ShotDistSq = ShotResult.GetShotDistanceSquared();
		const auto RatioSq = ShotDistSq / DistToFocusSq;

		if(RatioSq >= FMath::Square(MinDistanceFractionSuccess))
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Shot %d was successful. ShotDist=%.1fm; DistToFocus=%.1fm; Ratio=%.2f; MinDistanceFractionSuccess=%.2f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor),
				i + 1, FMath::Sqrt(ShotDistSq) / 100, FMath::Sqrt(DistToFocusSq) / 100, FMath::Sqrt(RatioSq), MinDistanceFractionSuccess);
			continue;
		}

		++ConsecutiveShotCount;

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose,
			TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - Shot %d was a failure; retry %d/%d; DistanceSqToShotResult=%.1fm; ShotDist=%.1fm; DistToFocus=%.1fm; Ratio=%.2f; MinDistanceFractionSuccess=%.2f; Accuracy=%f; PowerFractionDeviation=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), 
			i + 1, ConsecutiveShotCount, MaxFailures,
			FMath::Sqrt(DistanceSqToShotResult) / 100, FMath::Sqrt(ShotDistSq) / 100, FMath::Sqrt(DistToFocusSq) / 100, FMath::Sqrt(RatioSq), MinDistanceFractionSuccess,
			FlickParams.Accuracy, ShotResult.ShotSetupParams.GetPowerDeviation());

		if (ConsecutiveShotCount >= MaxFailures)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
				TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - FALSE - FocusActor has had %d consecutive failures"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), ConsecutiveShotCount);
			SetOutFailures();
			return false;
		}
	} // for

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
		TEXT("%s-%s: IsFocusActorViableBasedOnShotHistory(%s) - TRUE - Exhausted shot history before triggering failure condition"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(CurrentFocusActor), 0);

	SetOutFailures();
	return true;
}

TOptional<UGolfAIShotComponent::FShotPowerCalculationResult> UGolfAIShotComponent::CalculateInitialShotParams() const
{
	if (!ensure(ShotContext.PlayerPawn))
	{
		return {};
	}

	auto PlayerPawn = ShotContext.PlayerPawn;

	// Predict flick to focus actor
	if (!ensure(FocusActor))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: CalculateInitialShotParams - FocusActor is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return {};
	}

	UE_VLOG_LOCATION(GetOwner(), LogPGAI, Log, FocusActor->GetActorLocation(), 10.0f, FColor::Green, TEXT("Target"));

	auto World = GetWorld();
	if (!ensure(World))
	{
		return {};
	}

	// Hitting at 45 degrees so can simplify the projectile calculation
	// https://en.wikipedia.org/wiki/Projectile_motion

	const auto ShotType = ShotContext.ShotType;

	const auto FlickLocation = PlayerPawn->GetFlickLocation(0.0f);
	const auto RawFlickMaxForce = PlayerPawn->GetFlickMaxForce(ShotType);
	// Account for drag
	//const auto FlickMaxForce = PlayerPawn->GetFlickDragForceMultiplier(RawFlickMaxForce) * RawFlickMaxForce;
	const auto FlickMaxForce = RawFlickMaxForce;
	const auto FlickMaxSpeed = FlickMaxForce / PlayerPawn->GetMass();

	// See https://en.wikipedia.org/wiki/Range_of_a_projectile
	// solve for v: d = v * cos(45 degrees) / g * (v * sin(45 degrees) + sqrt(v^2 * sin(45 degrees)^2 + 2 * g * y))
	// Using wolfram alpha to solve the equation when theta is 45 for v, we get
	// v = d * sqrt(g) / sqrt(d + y)
	// where d is the horizontal distance XY and y is the vertical distance Z and v is vxy
	// If we wanted to compute the velocity for an arbitrary angle we would use
	// v = d * sqrt(g) * sqrt(sec(theta)) / sqrt(2 * d * sin(theta) + 2 * y * cos(theta))
	// When theta = 0 degrees, then v = d * sqrt(g * y) / (sqrt(2) * y)
	// Angle of reach to get distance d is sin(2 * theta) = g * d / v^2
	const auto& FocusActorLocation = GetFocusActorLocation(FlickLocation);

	const auto PositionDelta = FocusActorLocation - FlickLocation;
	const auto HorizontalDistance = PositionDelta.Size2D();
	const auto VerticalDistance = PositionDelta.Z;
	const auto DistanceSum = CalculateDistanceSum(HorizontalDistance, VerticalDistance);
	const auto AdjustedHorizontalDistance = FMath::Min(HorizontalDistance, DistanceSum);

	float PowerFraction;

	// if TotalHorizontalDistance + VerticalDistance <= 0 then we use minimum power as so far above the target though this shouldn't happen based on updated CalculateDistanceSum
	if (DistanceSum <= 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateInitialShotParams - Way above target, using min force. HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), HorizontalDistance / 100, VerticalDistance / 100);

		PowerFraction = MinShotPower;
	}
	else
	{
		const auto Gravity = FMath::Abs(World->GetGravityZ());
		const auto Speed = AdjustedHorizontalDistance * FMath::Sqrt(Gravity) / FMath::Sqrt(DistanceSum);

		// Compare speed to max flick speed
		// Impulse is proportional to sqrt of the distance ratio if overshoot

		if (Speed >= FlickMaxSpeed)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
				TEXT("%s-%s: CalculateInitialShotParams - Coming up short hit full power - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm; DistanceSum=%.1fm"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100, DistanceSum / 100);

			PowerFraction = 1.0f;
		}
		else
		{
			// also account for bounce distance
			const auto ReductionRatio = Speed / FlickMaxSpeed * (1 - BounceOverhitCorrectionFactor);

			// Impulse is proportional to sqrt of the velocity ratio
			const auto RawPowerFraction = FMath::Sqrt(ReductionRatio);
			PowerFraction = FMath::Max(MinShotPower, RawPowerFraction);

			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log,
				TEXT("%s-%s: CalculateInitialShotParams - Overhit - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm; DistanceSum=%.1fm; RawPowerFraction=%.2f; PowerFraction=%.2f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100, DistanceSum / 100, RawPowerFraction, PowerFraction);
		}
	}

	// Account for drag by increasing the power
	const auto FlickDragForceMultiplier = PlayerPawn->GetFlickDragForceMultiplier(PowerFraction * FlickMaxForce);
	PowerFraction = FMath::Clamp(PowerFraction / FlickDragForceMultiplier, 0.0f, 1.0f);

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateInitialShotParams - ShotType=%s; Power=%.2f; FlickDragForceMultiplier=%1.f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(ShotType),
		PowerFraction, FlickDragForceMultiplier);

	return FShotPowerCalculationResult
	{
		.FlickLocation = FlickLocation,
		.FlickMaxSpeed = FlickMaxSpeed,
		.PowerFraction = PowerFraction,
		.ShotType = ShotType
	};
}

double UGolfAIShotComponent::CalculateDistanceSum(double HorizontalDistance, double VerticalDistance) const
{
	if (VerticalDistance >= -MinHeightAdjustmentTreshold)
	{
		return HorizontalDistance + VerticalDistance;
	}

	// Reduce to account for projectile motion, but sometimes there is a surface that we need to "clear"
	// Ideally, we would calculate the distance to get �to the edge� by setting a target there and then basing the power calculation that we just need to clear the edge 
	// (and don�t correct for bounce), i.e. the distance needs to be at least the distance to the edge, so that we go over and bounce on the ground.

	const auto HorizontalDistanceMeters = HorizontalDistance / 100;
	const auto VerticalDistanceAbsMeters = FMath::Abs(VerticalDistance) / 100;
	const auto HorizontalRatio = HorizontalDistanceMeters / VerticalDistanceAbsMeters;

	const auto AdjustedVerticalDistance = -100 * (VerticalDistanceAbsMeters - FMath::Pow(VerticalDistanceAbsMeters, 1 / (HorizontalRatio + HeightToDistanceRatioStartExp)));

	// Make sure distance is at least the min factor of the horizontal distance
	const auto NewSum = HorizontalDistance + AdjustedVerticalDistance;
	const auto ResultDistance = FMath::Max(HorizontalDistance * MinHeightDistanceReductionFactor, NewSum);

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateDistanceSum - ADJUSTED - HorizontalDistance=%.1fm; VerticalDistance=%.1fm; AdjustedVerticalDistance=%.1fm -> %.1fm"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), HorizontalDistance / 100, VerticalDistance / 100, AdjustedVerticalDistance / 100, ResultDistance / 100);

	return ResultDistance;
}

FVector UGolfAIShotComponent::GetFocusActorLocation(const FVector& FlickLocation) const
{
	auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	// Predict flick to focus actor
	check(FocusActor);

	// Prefer focus actor unless the hole is closer
	const auto& DefaultLocation = FocusActor->GetActorLocation();
	const auto FocusDistanceSq = (DefaultLocation - FlickLocation).SizeSquared2D();

	auto GolfHole = ShotContext.GolfHole;
	check(GolfHole);

	const auto HoleLocation = GolfHole->GetActorLocation();
	const auto HoleDistanceSq = (HoleLocation - FlickLocation).SizeSquared2D();

	if(FocusDistanceSq <= HoleDistanceSq)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: GetFocusActorLocation - Using focus actor as FocusActor=%s is closer than hole; FocusDistance=%.1fm; HoleDistance=%.1fm"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FocusActor), FMath::Sqrt(FocusDistanceSq) * 100, FMath::Sqrt(HoleDistanceSq) * 100);
		return DefaultLocation;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: GetFocusActorLocation - Using hole as it is closer than FocusActor=%s; FocusDistance=%.1fm; HoleDistance=%.1fm"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FocusActor), FMath::Sqrt(FocusDistanceSq) * 100, FMath::Sqrt(HoleDistanceSq) * 100);

	return HoleLocation;
}

bool UGolfAIShotComponent::ValidateAndLoadConfig()
{
	bool bValid = true;

	if (!ValidateCurveTable(AIConfigCurveTable))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: ValidateConfig - FALSE - AIConfigCurveTable is invalid"), *LoggingUtils::GetName(GetOwner()), *GetName());
		bValid = false;
	}

	if (!ValidateAndLoadAIPerformanceStrategy())
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: ValidateConfig - FALSE - AIPerformanceStrategy is invalid"), *LoggingUtils::GetName(GetOwner()), *GetName());
		bValid = false;
	}

	if (bValid)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: ValidateConfig - TRUE"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	return bValid;
}

bool UGolfAIShotComponent::ValidateAndLoadAIPerformanceStrategy()
{
	if (!ensure(AIPerformanceStrategyClass))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: ValidateAndLoadAIPerformanceStrategy - FALSE - AIPerformanceStrategyClass is NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
		return false;
	}

	AIPerformanceStrategy = NewObject<UAIPerformanceStrategy>(this, AIPerformanceStrategyClass);

	if (!ensure(AIPerformanceStrategy))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Error, TEXT("%s-%s: ValidateAndLoadAIPerformanceStrategy - FALSE - AIPerformanceStrategy=%s could not be created"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(AIPerformanceStrategyClass));
		return false;
	}

	return AIPerformanceStrategy->Initialize(
		PG::FAIPerformanceConfig {
			.MinPower = MinShotPower,
			.DefaultAccuracyDeviation = DefaultAccuracyDeviation,
			.DefaultPowerDeviation = DefaultPowerDeviation
	});
}

bool UGolfAIShotComponent::ValidateCurveTable(UCurveTable* CurveTable) const
{
	if (!ensure(CurveTable))
	{
		return false;
	}

	// Make sure can find the required curve tables
	return FindCurveForKey(CurveTable, DeltaDistanceMetersVsPowerFraction)  &&
		   FindCurveForKey(CurveTable, PowerFractionVsPitchAngle) &&
		   FindCurveForKey(CurveTable, DeltaDistanceMetersVsZOffset) &&
		   FindCurveForKey(CurveTable, AngleDeviationVsPowerMultiplier);
}

float UGolfAIShotComponent::CalculateDefaultZOffset() const
{
	switch (ShotContext.ShotType)
	{
	case EShotType::Close:
			return MinZOffset;
	case EShotType::Medium:
		return MinZOffset / 2;
	default: //FullShot
		return MaxZOffset;
	}
}
UGolfAIShotComponent::FShotCalculationResult UGolfAIShotComponent::CalculateAvoidanceShotFactors(const FVector& FlickLocation, float PreferredPitchAngle, float FlickMaxSpeed, float PowerFraction) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UGolfAIShotComponent::CalculateAvoidanceShotFactors");

	const auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	const auto GolfHole = ShotContext.GolfHole;
	check(GolfHole);

	const auto AdditionalRotationYaw = PG::MathUtils::ClampDeltaYaw(PlayerPawn->GetRotationYawToFocusActor(FocusActor) - InitialFocusYaw);
	const auto& ActorUpVector = PlayerPawn->GetActorUpVector();

	const FRotator AdditionalActorRotation = { 0, PG::MathUtils::ClampDeltaYaw(PlayerPawn->GetRotationYawToFocusActor(FocusActor) - InitialFocusYaw), 0 };
	const auto& OriginalFlickDirection = PlayerPawn->GetFlickDirection();
	const auto& DefaultFlickDirection = OriginalFlickDirection.RotateAngleAxis(AdditionalRotationYaw, ActorUpVector);

	auto GetPreferredYawDirection = [&, PreferredDirectionOptional = TOptional<FVector>{}]() mutable
	{
		if (PreferredDirectionOptional.IsSet())
		{
			return PreferredDirectionOptional.GetValue();
		}
		// Calculate the average direction to all the focus actors in front of us
		FVector Direction{ EForceInit::ForceInitToZero };
		const auto HoleDirection = (GolfHole->GetActorLocation() - FlickLocation).GetSafeNormal();

#if ENABLE_VISUAL_LOG || !NO_LOGGING
		TArray<AActor*, TInlineAllocator<8>> RelevantFocusActors;
#endif

		bool bAddedDirection{};
		for (const auto& FocusActorScore : ShotContext.FocusActorScores)
		{
			const auto FocusActor = FocusActorScore.FocusActor;
			if (!FocusActor || FocusActor == GolfHole)
			{
				continue;
			}

			auto ToFocusActor = FocusActor->GetActorLocation() - FlickLocation;
			if ((ToFocusActor | HoleDirection) > 0)
			{
#if ENABLE_VISUAL_LOG || !NO_LOGGING
				RelevantFocusActors.Add(FocusActor);
#endif
				Direction += ToFocusActor.GetSafeNormal();
				bAddedDirection = true;
			}
		}

		// Add the hole direction if there are no other options
		if (!bAddedDirection)
		{
			Direction += HoleDirection;
		}

		PreferredDirectionOptional = Direction.GetSafeNormal();

		UE_VLOG_ARROW(GetOwner(), LogPGAI, Log, FlickLocation, FlickLocation + Direction * 100.0f, FColor::Emerald, TEXT("PreferredYawDirection"));
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotFactors - PreferredYawDirection=%s; Relevant focus actors=[%s]"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), *PreferredDirectionOptional->ToCompactString(), *PG::ToStringObjectElements(RelevantFocusActors));

		return *PreferredDirectionOptional;
	};

	const auto& HoleDirection = (GolfHole->GetActorLocation() - FlickLocation).GetSafeNormal();

	UE_VLOG_ARROW(GetOwner(), LogPGAI, Log, FlickLocation, FlickLocation + ActorUpVector * 100.0f, FColor::Blue, TEXT("Up"));

	// Via the work energy principle, reducing the power by N will reduce the speed by factor of sqrt(N)
	const auto FlickSpeed = FlickMaxSpeed * FMath::Sqrt(PowerFraction);

	if (UPaperGolfPawnUtilities::TraceShotAngle(this, PlayerPawn, FlickLocation, DefaultFlickDirection, FlickSpeed, PreferredPitchAngle, MinTraceDistance))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotFactors - Solution Found with preferred pitch - PreferredPitch=%.1f; AdditionalRotationYaw=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PreferredPitchAngle, AdditionalRotationYaw);
		return { PreferredPitchAngle, 0.0f, 1.0f };
	}

	FShotCalculationResult LastResult{};

	// If last shot ended in hazard, adjust yaw to try and get around it
	const int32 DesiredSkips = bCurrentFocusActorLandedInHazard ? CurrentFocusActorFailures : 0;

	for (int32 PreferredDirection = 1, Increments = 2 * FMath::FloorToInt32(180.0f / YawRetryDelta), i = FMath::Max(0, FMath::Min(DesiredSkips, Increments - 1)); i < Increments; ++i)
	{
		if (i == 1)
		{
			// skip retrying yaw 0
			continue;
		}

		// calculate preferred direction for the first time
		TArray<FVector, TInlineAllocator<2>> CalculatedDirections;
		if (i == 2)
		{
			// Pick direction that most aligns with hole
			const auto PositiveRotation = DefaultFlickDirection.RotateAngleAxis(YawRetryDelta, ActorUpVector);
			CalculatedDirections.Add(PositiveRotation);

			const auto NegativeRotation = DefaultFlickDirection.RotateAngleAxis(-YawRetryDelta, ActorUpVector);
			CalculatedDirections.Add(NegativeRotation);

			const auto& PreferredYawDirection = GetPreferredYawDirection();
			const auto PositiveDot = PositiveRotation | PreferredYawDirection;
			const auto NegativeDot = NegativeRotation | PreferredYawDirection;

			PreferredDirection = PositiveDot >= NegativeDot ? 1 : -1;

			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose, TEXT("%s-%s: CalculateShotFactors - PreferredDirection=%d; PositiveDot=%.2f; NegativeDot=%.2f; PositionRotation=%s; NegativeRotation=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), PreferredDirection, PositiveDot, NegativeDot, *PositiveRotation.ToCompactString(), *NegativeRotation.ToCompactString());
		}

		// Try positive and then negative version of angle
		const auto Yaw = YawRetryDelta * (i / 2) * (i % 2 == 0 ? PreferredDirection : -PreferredDirection);
		const FVector FlickDirectionRotated = CalculatedDirections.IsEmpty() ? DefaultFlickDirection.RotateAngleAxis(Yaw, ActorUpVector) : CalculatedDirections[PreferredDirection > 0 ? 0 : 1];

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose, TEXT("%s-%s: CalculateShotFactors - i=%d/%d; Yaw=%.1f; FlickDirectionRotated=%s"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), i, Increments - 1, Yaw, *FlickDirectionRotated.ToCompactString());
		const auto [bPass, Pitch ] = CalculateShotPitch(FlickLocation, FlickDirectionRotated, FlickSpeed);
		LastResult = { Pitch, Yaw, static_cast<float>(DefaultFlickDirection | FlickDirectionRotated)};

		if (bPass)
		{
			UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotFactors - Solution Found - Yaw=%.1f; Pitch=%.1f; PowerReduction=%1.f"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), Yaw, Pitch, LastResult.PowerMultiplier);
			return LastResult;
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Display, TEXT("%s-%s: CalculateShotFactors - NO SOLUTION - Default to Yaw=%.1f; Pitch=%.1f; PowerReduction=%.1f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), LastResult.Yaw, LastResult.Pitch, LastResult.PowerMultiplier);

	return LastResult;
}

TTuple<bool, float> UGolfAIShotComponent::CalculateShotPitch(const FVector& FlickLocation, const FVector& FlickDirection, float FlickSpeed) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UGolfAIShotComponent::CalculateShotPitch");

	static_assert(ShotPitchAngles.size() > 0, "ShotPitchAngles must have at least one element");

	if constexpr(ShotPitchAngles.size() == 1)
	{
		return { true, ShotPitchAngles.front() };
	}
	else
	{
		const auto PlayerPawn = ShotContext.PlayerPawn;
		check(PlayerPawn);

		auto World = GetWorld();
		check(World);

		int32 NumSkips{};
		// Don't adjust pitch if last shot landed in hazard as we want to maximize displacement to get over it and instead adjust yaw
		const int32 DesiredSkips = bCurrentFocusActorLandedInHazard ? 0 : CurrentFocusActorFailures;

		for (auto It = ShotPitchAngles.begin(), End = std::next(ShotPitchAngles.begin(), ShotPitchAngles.size() - 1); It != End; ++It)
		{
			const auto PitchAngle = *It;

			bool bPass = UPaperGolfPawnUtilities::TraceShotAngle(this, PlayerPawn, FlickLocation, FlickDirection, FlickSpeed, PitchAngle, MinTraceDistance);
			// If the test suggests it will pass but it failed due to this being a retry then skip to next
			// Ideally this would also change the yaw to avoid the obstacle but the trace test should catch a majority of the issues
			if (bPass && NumSkips >= DesiredSkips)
			{
				return { true, PitchAngle };
			}
			else if(bPass)
			{
				++NumSkips;
			}
		}

		// return default angle, which is one with largest angle
		const auto MaxAngleElement = std::max_element(ShotPitchAngles.begin(), ShotPitchAngles.end());
		check(MaxAngleElement != ShotPitchAngles.end());

		return { false, *MaxAngleElement };
	}
}

UGolfAIShotComponent::FShotSetupParams UGolfAIShotComponent::CalculateDefaultShotParams() const
{
	const auto Box = PG::CollisionUtils::GetAABB(*ShotContext.PlayerPawn);
	const auto ZExtent = Box.GetExtent().Z;

	// Hit full power
	const FFlickParams FlickParams =
	{
		.ShotType = ShotContext.ShotType,
		.LocalZOffset = 0,
		.PowerFraction = 1.0f,
		.Accuracy = PG::GolfAIShotCalculationUtils::GenerateAccuracy(-DefaultAccuracyDeviation, DefaultAccuracyDeviation)
	};

	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset);

	return
	{
		.FlickLocation = ShotContext.PlayerPawn->GetFlickLocation(FlickParams.LocalZOffset),
		.InitialPowerFraction = FlickParams.PowerFraction,
		.ShotSetupResult = FAIShotSetupResult
		{
			.FlickParams = FlickParams,
			.FocusActor = FocusActor,
			.ShotPitch = DefaultPitchAngle
		}
	};
}

UGolfAIShotComponent::FShotCalibrationResult UGolfAIShotComponent::CalibrateShot(const FVector& FlickLocation, float PowerFraction) const
{
	if (!AIConfigCurveTable)
	{
		return FShotCalibrationResult
		{
			.PowerFraction = PowerFraction,
			.Pitch = DefaultPitchAngle
		};
	}

	float PitchAngle = DefaultPitchAngle;

	auto Curve = FindCurveForKey(AIConfigCurveTable, DeltaDistanceMetersVsPowerFraction);

	auto DistanceToHoleCalculator = [this, &FlickLocation, Distance = -1.0f]() mutable
	{
		// memoize result
		if (Distance < 0.0f)
		{
			check(ShotContext.GolfHole);
			const auto& HoleLocation = ShotContext.GolfHole->GetActorLocation();

			Distance = (HoleLocation - FlickLocation).Size2D() / 100;
		}
		return Distance;
	};

	if (Curve)
	{
		const auto DistanceToHoleMeters = DistanceToHoleCalculator();
		const auto PowerReductionFactor = Curve->Eval(DistanceToHoleMeters);

		const auto NewPowerFractionCalc = [&]()
		{
			return FMath::Max(MinShotPower, PowerFraction * PowerReductionFactor);
		};

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalibrateShot - DistanceToHole=%.1fm; PowerReductionFactor=%.2f; PowerFraction=%.2f -> %.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), DistanceToHoleMeters, PowerReductionFactor, PowerFraction, NewPowerFractionCalc());

		// Make sure don't fall below min shot power
		PowerFraction = NewPowerFractionCalc();
	}

	Curve = FindCurveForKey(AIConfigCurveTable, PowerFractionVsPitchAngle);
	if (Curve)
	{
		PitchAngle = Curve->Eval(PowerFraction);

		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalibrateShot - PowerFraction=%.2f -> PitchAngle=%.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PowerFraction, PitchAngle);
	}

	Curve = FindCurveForKey(AIConfigCurveTable, DeltaDistanceMetersVsZOffset);

	float LocalZOffset;
	if (Curve)
	{
		const auto DistanceToHoleMeters = DistanceToHoleCalculator();
		LocalZOffset = Curve->Eval(DistanceToHoleMeters);
	}
	else
	{
		LocalZOffset = CalculateDefaultZOffset();
	}

	return FShotCalibrationResult
	{
		.PowerFraction = PowerFraction,
		.Pitch = PitchAngle,
		.LocalZOffset = LocalZOffset
	};
}

FString FAIShotContext::ToString() const
{
	return FString::Printf(TEXT("PlayerPawn=%s; PlayerState=%s; ShotType=%s"),
		*LoggingUtils::GetName(PlayerPawn), *LoggingUtils::GetName<APlayerState>(PlayerState.Get()), *LoggingUtils::GetName(ShotType));
}

namespace
{
	FRealCurve* FindCurveForKey(UCurveTable* CurveTable, const FName& Key)
	{
		check(CurveTable);

#if WITH_EDITOR
		FString Context = FString::Printf(TEXT("FindCurveForKey: %s -> %s"), *LoggingUtils::GetName(CurveTable), *Key.ToString());
		return CurveTable->FindCurve(Key, Context, true);
#else
		return CurveTable->FindCurveUnchecked(Key);
#endif
	}

	FVector GetVelocityAtHitPoint(const FPredictProjectilePathResult& PathResult)
	{
		const auto& HitLocation = PathResult.HitResult.ImpactPoint;

		FVector Velocity{ EForceInit::ForceInitToZero };
		double MinDist{ std::numeric_limits<double>::max() };

		const auto& PathData = PathResult.PathData;

		for (int i = PathData.Num() - 1; i >= 0; --i)
		{
			const auto& Point = PathData[i];

			const auto Dist = FVector::DistSquared(HitLocation, Point.Location);
			if (Dist < MinDist)
			{
				MinDist = Dist;
				Velocity = Point.Velocity;

				// Close enough - break at this point
				if (Dist <= DistSqHitThreshold)
				{
					break;
				}
			}
		}

		return Velocity;
	}
}

FString UGolfAIShotComponent::FShotSetupParams::ToString() const
{
	return FString::Printf(TEXT("FlickLocation=%s; InitialPowerFraction=%f; FinalFlickParams=[%s]; FocusActor=%s; ShotPitch=%.1f; ShotYaw=%.1f"),
		*FlickLocation.ToCompactString(), InitialPowerFraction, *ShotSetupResult.FlickParams.ToString(), *LoggingUtils::GetName(ShotSetupResult.FocusActor), ShotSetupResult.ShotPitch, ShotSetupResult.ShotYaw);
}

float UGolfAIShotComponent::FShotResult::GetShotDistanceSquared() const
{
	return FVector::DistSquared(ShotSetupParams.FlickLocation, EndPosition);
}
