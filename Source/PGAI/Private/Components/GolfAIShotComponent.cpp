// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/GolfAIShotComponent.h"


#include "Pawn/PaperGolfPawn.h"

#include "State/GolfPlayerState.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGAILogging.h"
#include "Utils/CollisionUtils.h"

#include "Subsystems/GolfEventsSubsystem.h"

#include "Kismet/GameplayStatics.h"

#include "Engine/CurveTable.h"
#include "Curves/RealCurve.h"
#include "Data/GolfAIConfigData.h"

#include <array>
#include <iterator>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIShotComponent)

namespace
{
	constexpr const std::array ShotPitchAngles = { 45.0f, 60.0f, 75.0f, 30.0f, 0.0f, -15.0f };

	// Gets the power fraction for a given delta distance in meters from the hole
	const FName DeltaDistanceMetersVsPowerFraction = TEXT("DeltaDistanceM_Power");

	// Gets the pitch angle to hit the shot vs the power fraction
	const FName PowerFractionVsPitchAngle = TEXT("Power_Angle");

	FRealCurve* FindCurveForKey(UCurveTable* CurveTable, const FName& Key);
	
	constexpr float DefaultPitchAngle = 45.0f;
}

UGolfAIShotComponent::UGolfAIShotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UGolfAIShotComponent::FAIShotSetupResult UGolfAIShotComponent::SetupShot(const FAIShotContext& InShotContext)
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: SetupShot - ShotContext=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *InShotContext.ToString());

	ShotContext = InShotContext;

	auto ShotParams = CalculateShotParams();
	if (!ShotParams)
	{
		ShotParams = CalculateDefaultShotParams();
	}

	check(ShotParams);

	ShotContext = {};

	return *ShotParams;
}

void UGolfAIShotComponent::BeginPlay()
{
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());

	Super::BeginPlay();

	ValidateAndLoadConfig();
}

TOptional<UGolfAIShotComponent::FAIShotSetupResult> UGolfAIShotComponent::CalculateShotParams()
{
	if (!ensure(ShotContext.PlayerPawn))
	{
		return {};
	}

	auto PlayerPawn = ShotContext.PlayerPawn;

	// Predict flick to focus actor
	auto FocusActor = PlayerPawn->GetFocusActor();
	if (!ensure(FocusActor))
	{
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s: CalculateShotParams - FocusActor is NULL"), *GetName());
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

	FFlickParams FlickParams;
	FlickParams.ShotType = ShotContext.ShotType;
	FlickParams.LocalZOffset = CalculateZOffset();

	const auto FlickLocation = PlayerPawn->GetFlickLocation(FlickParams.LocalZOffset);
	const auto RawFlickMaxForce = PlayerPawn->GetFlickMaxForce(FlickParams.ShotType);
	// Account for drag
	//const auto FlickMaxForce = PlayerPawn->GetFlickDragForceMultiplier(RawFlickMaxForce) * RawFlickMaxForce;
	const auto FlickMaxForce = RawFlickMaxForce;
	const auto FlickMaxSpeed = FlickMaxForce / PlayerPawn->GetMass();

	// See https://en.wikipedia.org/wiki/Range_of_a_projectile
	// Using wolfram alpha to solve the equation when theta is 45 for v, we get
	// v = d * sqrt(g) / sqrt(d + y)
	// where d is the horizontal distance XY and y is the vertical distance Z and v is vxy
	const auto& FocusActorLocation = GetFocusActorLocation(FlickLocation);

	const auto PositionDelta = FocusActorLocation - FlickLocation;
	const auto HorizontalDistance = PositionDelta.Size2D();
	const auto VerticalDistance = PositionDelta.Z;
	const auto DistanceSum = HorizontalDistance + VerticalDistance;

	float PowerFraction;

	// if TotalHorizontalDistance + VerticalDistance <= 0 then we use minimum power as so far above the target
	if (DistanceSum <= 0)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateShotParams - Way above target, using min force. HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
			*GetName(), HorizontalDistance / 100, VerticalDistance / 100);

		PowerFraction = MinShotPower;
	}
	else
	{
		const auto Gravity = FMath::Abs(World->GetGravityZ());
		const auto Speed = HorizontalDistance * FMath::Sqrt(Gravity) / FMath::Sqrt(DistanceSum);

		// Compare speed to max flick speed
		// Impulse is proportional to sqrt of the distance ratio if overshoot

		if (Speed >= FlickMaxSpeed)
		{
			UE_VLOG_UELOG(this, LogPGAI, Log,
				TEXT("%s: CalculateShotParams - Coming up short hit full power - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm"),
				*GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100);

			PowerFraction = 1.0f;
		}
		else
		{
			// also account for bounce distance
			const auto ReductionRatio = Speed / FlickMaxSpeed * (1 - BounceOverhitCorrectionFactor);

			// Impulse is proportional to sqrt of the velocity ratio
			const auto RawPowerFraction = FMath::Sqrt(ReductionRatio);
			PowerFraction = FMath::Max(MinShotPower, RawPowerFraction);

			UE_VLOG_UELOG(this, LogPGAI, Log,
				TEXT("%s: CalculateShotParams - Overhit - Speed=%.1f; FlickMaxSpeed=%.1f, HorizontalDistance=%.1fm; VerticalDistance=%.1fm; RawPowerFraction=%.2f; PowerFraction=%.2f"),
				*GetName(), Speed, FlickMaxSpeed, HorizontalDistance / 100, VerticalDistance / 100, RawPowerFraction, PowerFraction);
		}
	}

	// Account for drag by increasing the power
	const auto FlickDragForceMultiplier = PlayerPawn->GetFlickDragForceMultiplier(PowerFraction * FlickMaxForce);
	PowerFraction = FMath::Clamp(PowerFraction / FlickDragForceMultiplier, 0.0f, 1.0f);

	const auto ShotCalibrationResult = CalibrateShot(FlickLocation, PowerFraction);
	PowerFraction = ShotCalibrationResult.PowerFraction;

	// Add error to power calculation and accuracy
	const auto FlickError = CalculateShotError(PowerFraction);
	FlickParams.PowerFraction = FlickError.PowerFraction;
	FlickParams.Accuracy = FlickError.Accuracy;

	const auto& ShotCalculationResult = CalculateShotFactors(FlickLocation, ShotCalibrationResult.Pitch, FlickMaxSpeed, FlickParams.PowerFraction);

	FlickParams.PowerFraction *= FMath::Max(ShotCalculationResult.PowerMultiplier, MinRetryShotPowerReductionFactor);

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateShotParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f; ShotPitch=%.1f; ShotYaw=%.1f; FlickDragForceMultiplier=%1.f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType),
		FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset, ShotCalculationResult.Pitch, ShotCalculationResult.Yaw, FlickDragForceMultiplier);

	return FAIShotSetupResult
	{
		.FlickParams = FlickParams,
		.ShotPitch = ShotCalculationResult.Pitch,
		.ShotYaw = ShotCalculationResult.Yaw
	};
}

FVector UGolfAIShotComponent::GetFocusActorLocation(const FVector& FlickLocation) const
{
	auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	// Predict flick to focus actor
	auto FocusActor = PlayerPawn->GetFocusActor();
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
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: GetFocusActorLocation - Using focus actor as FocusActor=%s is closer than hole; FocusDistance=%.1fm; HoleDistance=%.1fm"),
			*GetName(), *LoggingUtils::GetName(FocusActor), FMath::Sqrt(FocusDistanceSq) * 100, FMath::Sqrt(HoleDistanceSq) * 100);
		return DefaultLocation;
	}

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: GetFocusActorLocation - Using hole as it is closer than FocusActor=%s; FocusDistance=%.1fm; HoleDistance=%.1fm"),
		*GetName(), *LoggingUtils::GetName(FocusActor), FMath::Sqrt(FocusDistanceSq) * 100, FMath::Sqrt(HoleDistanceSq) * 100);

	return HoleLocation;
}

bool UGolfAIShotComponent::ValidateAndLoadConfig()
{
	bool bValid = true;

	AIErrorsData = GolfAIConfigDataParser::ReadAll(AIErrorsDataTable);
	if (AIErrorsData.IsEmpty())
	{
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s-%s: ValidateConfig - FALSE - AIErrorsData is empty"), *LoggingUtils::GetName(GetOwner()), *GetName());
		bValid = false;
	}

	if (!ValidateCurveTable(AIConfigCurveTable))
	{
		UE_VLOG_UELOG(this, LogPGAI, Error, TEXT("%s-%s: ValidateConfig - FALSE - AIConfigCurveTable is invalid"), *LoggingUtils::GetName(GetOwner()), *GetName());
		bValid = false;
	}

	if (bValid)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: ValidateConfig - TRUE"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	return bValid;
}

bool UGolfAIShotComponent::ValidateCurveTable(UCurveTable* CurveTable) const
{
	if (!ensure(CurveTable))
	{
		return false;
	}

	// Make sure can find the required curve tables
	return FindCurveForKey(CurveTable, DeltaDistanceMetersVsPowerFraction)  &&
		   FindCurveForKey(CurveTable, PowerFractionVsPitchAngle);
}

float UGolfAIShotComponent::GenerateAccuracy(float Deviation) const
{
	return FMath::FRandRange(-Deviation, Deviation);
}

float UGolfAIShotComponent::GeneratePowerFraction(float InPowerFraction, float Deviation) const
{
	return FMath::Clamp(InPowerFraction * (1 + FMath::RandRange(-Deviation, Deviation)), MinShotPower, 1.0f);
}

float UGolfAIShotComponent::CalculateZOffset() const
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
UGolfAIShotComponent::FShotCalculationResult UGolfAIShotComponent::CalculateShotFactors(const FVector& FlickLocation, float PreferredPitchAngle, float FlickMaxSpeed, float PowerFraction) const
{
	const auto PlayerPawn = ShotContext.PlayerPawn;
	check(PlayerPawn);

	const auto GolfHole = ShotContext.GolfHole;
	check(GolfHole);

	const auto& DefaultFlickDirection = PlayerPawn->GetFlickDirection();
	const auto& ActorUpVector = PlayerPawn->GetActorUpVector();
	const auto& HoleDirection = (GolfHole->GetActorLocation() - FlickLocation).GetSafeNormal();

	UE_VLOG_ARROW(GetOwner(), LogPGAI, Log, FlickLocation, FlickLocation + ActorUpVector * 100.0f, FColor::Blue, TEXT("Up"));

	// Via the work energy principle, reducing the power by N will reduce the speed by factor of sqrt(N)
	const auto FlickSpeed = FlickMaxSpeed * FMath::Sqrt(PowerFraction);

	if (TraceShotAngle(*PlayerPawn, FlickLocation, DefaultFlickDirection, FlickSpeed, PreferredPitchAngle))
	{
		UE_VLOG_UELOG(GetOwner(), LogPGAI, Log, TEXT("%s-%s: CalculateShotFactors - Solution Found with preferred pitch - PreferredPitch=%.1f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PreferredPitchAngle);
		return { PreferredPitchAngle, 0.0f, 1.0f };
	}

	FShotCalculationResult LastResult{};

	for (int32 i = 0, PreferredDirection = 1, Increments = 2 * FMath::FloorToInt32(180.0f / YawRetryDelta); i < Increments; ++i)
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

			const auto PositiveDot = PositiveRotation | HoleDirection;
			const auto NegativeDot = NegativeRotation | HoleDirection;

			PreferredDirection = PositiveDot >= NegativeDot ? 1 : -1;

			UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose, TEXT("%s-%s: CalculateShotFactors - PreferredDirection=%d; PositiveDot=%.2f; NegativeDot=%.2f; PositionRotation=%s; NegativeRotation=%s"),
				*LoggingUtils::GetName(GetOwner()), *GetName(), PreferredDirection, PositiveDot, NegativeDot, *PositiveRotation.ToCompactString(), *NegativeRotation.ToCompactString());
		}

		// Try positive and then negative version of angle
		const auto Yaw = YawRetryDelta * (i / 2) * (i % 2 == 0 ? PreferredDirection : -PreferredDirection);
		const FVector FlickDirectionRotated = CalculatedDirections.IsEmpty() ? DefaultFlickDirection.RotateAngleAxis(Yaw, ActorUpVector) : CalculatedDirections[PreferredDirection > 0 ? 0 : 1];

		UE_VLOG_UELOG(this, LogPGAI, Verbose, TEXT("%s-%s: CalculateShotFactors - i=%d/%d; Yaw=%.1f; FlickDirectionRotated=%s"),
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

		for (auto It = ShotPitchAngles.begin(), End = std::next(ShotPitchAngles.begin(), ShotPitchAngles.size() - 1); It != End; ++It)
		{
			const auto PitchAngle = *It;

			bool bPass = TraceShotAngle(*PlayerPawn, FlickLocation, FlickDirection, FlickSpeed, PitchAngle);
			if (bPass)
			{
				return { true, PitchAngle };
			}
		}

		// return default angle, which is last element
		return { false, ShotPitchAngles.back() };
	}
}

bool UGolfAIShotComponent::TraceShotAngle(const APaperGolfPawn& PlayerPawn, const FVector& TraceStart, const FVector& FlickDirection, float FlickSpeed, float FlickAngleDegrees) const
{
	auto World = GetWorld();
	check(World);

	const auto MaxHeight = FlickAngleDegrees > 0 ? FMath::Max(GetMaxProjectileHeight(45.0f, FlickSpeed), MinTraceDistance) : MinTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(&PlayerPawn);

	// Pitch up or down based on the FlickAngleDegrees
	const auto PitchedFlickDirection = FlickDirection.RotateAngleAxis(FlickAngleDegrees, -PlayerPawn.GetActorRightVector());

	const FVector TraceEnd = TraceStart + PitchedFlickDirection * MaxHeight;

	// Try line trace directly to max height as an approximation
	bool bPass = !World->LineTraceTestByChannel(TraceStart, TraceEnd, PG::CollisionChannel::FlickTraceType, QueryParams);

	UE_VLOG_ARROW(GetOwner(), LogPGAI, Log, TraceStart, TraceEnd, bPass ? FColor::Green : FColor::Red, TEXT("Trace %.1f"), FlickAngleDegrees);
	UE_VLOG_UELOG(GetOwner(), LogPGAI, Verbose, TEXT("%s-%s: TraceShotAngle - TraceStart=%s; FlickDirection=%s; PitchedFlickDirection=%s; FlickAngle=%.1f; FlickSpeed=%.1f; MaxHeight=%.1f; bPass=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		*TraceStart.ToCompactString(), *FlickDirection.ToCompactString(), *PitchedFlickDirection.ToCompactString(), FlickAngleDegrees, FlickSpeed, MaxHeight, LoggingUtils::GetBoolString(bPass));

	return bPass;
}

float UGolfAIShotComponent::GetMaxProjectileHeight(float FlickPitchAngle, float FlickSpeed) const
{
	// H = v^2 * sin^2(theta) / 2g
	auto World = GetWorld();
	check(World);

	return FMath::Square(FlickSpeed) * FMath::Square(FMath::Sin(FMath::DegreesToRadians(FlickPitchAngle))) / (2 * FMath::Abs(World->GetGravityZ()));
}

UGolfAIShotComponent::FAIShotSetupResult UGolfAIShotComponent::CalculateDefaultShotParams() const
{
	const auto Box = PG::CollisionUtils::GetAABB(*ShotContext.PlayerPawn);
	const auto ZExtent = Box.GetExtent().Z;

	// Hit full power
	const FFlickParams FlickParams =
	{
		.ShotType = ShotContext.ShotType,
		.LocalZOffset = 0,
		.PowerFraction = 1.0f,
		.Accuracy = GenerateAccuracy(DefaultAccuracyDeviation)
	};

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateShotParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset);

	return FAIShotSetupResult
	{
		.FlickParams = FlickParams,
		.ShotPitch = DefaultPitchAngle
	};
}

UGolfAIShotComponent::FShotCalibrationResult UGolfAIShotComponent::CalibrateShot(const FVector& FlickLocation, float PowerFraction) const
{
	// TODO: Replace with curve table results
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
	if (Curve)
	{
		check(ShotContext.GolfHole);
		const auto& HoleLocation = ShotContext.GolfHole->GetActorLocation();

		const auto DistanceToHoleMeters = (HoleLocation - FlickLocation).Size2D() / 100;

		const auto PowerReductionFactor = Curve->Eval(DistanceToHoleMeters);

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalibrateShot - DistanceToHole=%.1fm; PowerReductionFactor=%.2f; PowerFraction=%.2f -> %.2f"),
			*GetName(), DistanceToHoleMeters, PowerReductionFactor, PowerFraction, PowerFraction * PowerReductionFactor);

		PowerFraction *= PowerReductionFactor;
	}

	Curve = FindCurveForKey(AIConfigCurveTable, PowerFractionVsPitchAngle);
	if (Curve)
	{
		PitchAngle = Curve->Eval(PowerFraction);

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalibrateShot - PowerFraction=%.2f -> PitchAngle=%.2f"),
			*GetName(), PowerFraction, PitchAngle);
	}

	return FShotCalibrationResult
	{
		.PowerFraction = PowerFraction,
		.Pitch = PitchAngle
	};
}

const FGolfAIConfigData* UGolfAIShotComponent::SelectAIConfigEntry() const
{
	const FGolfAIConfigData* Selected{};

	for (const auto& Entry : AIErrorsData)
	{
		if(ShotsSinceLastError <= Entry.ShotsSinceLastError)
		{
			Selected = &Entry;
			break;
		}
	}

	if (!Selected && !AIErrorsData.IsEmpty())
	{
		Selected = &AIErrorsData.Last();

		UE_VLOG_UELOG(this, LogPGAI, Warning, TEXT("%s-%s: SelectAIConfigEntry - ShotsSinceLastError=%d; Defaulted to last entry with Shots=%d"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), ShotsSinceLastError, Selected->ShotsSinceLastError);
	}

	return Selected;
}

UGolfAIShotComponent::FShotErrorResult UGolfAIShotComponent::CalculateShotError(float PowerFraction)
{
	const auto AIConfigEntry = SelectAIConfigEntry();
	if (!AIConfigEntry)
	{
		UE_VLOG_UELOG(this, LogPGAI, Warning, TEXT("%s-%s: CalculateShotError - AIConfigEntry is NULL - using defaults"), *LoggingUtils::GetName(GetOwner()), *GetName());

		return FShotErrorResult
		{
			.PowerFraction = GeneratePowerFraction(PowerFraction, DefaultPowerDeviation),
			.Accuracy = GenerateAccuracy(DefaultAccuracyDeviation),
		};
	}

	const auto PerfectShotRoll = FMath::FRand();

	if (PerfectShotRoll <= AIConfigEntry->PerfectShotProbability)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Perfect Shot - PerfectShotRoll=%.2f; PerfectShotProbability=%.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PerfectShotRoll, AIConfigEntry->PerfectShotProbability);

		++ShotsSinceLastError;

		return FShotErrorResult
		{
			.PowerFraction = PowerFraction,
			.Accuracy = 0.0f
		};
	}

	ShotsSinceLastError = 0;

	// Check for a "shank shot"
	const auto ShankShotRoll = FMath::FRand();

	if (ShankShotRoll <= AIConfigEntry->ShankShotProbability)
	{
		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Shank Shot - ShankShotRoll=%.2f; ShankShotProbability=%.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), ShankShotRoll, AIConfigEntry->ShankShotProbability);

		const auto ShankPowerFactor = AIConfigEntry->ShankPowerFactor;
		if (ShankPowerFactor * PowerFraction <= 1.0f)
		{
			PowerFraction *= ShankPowerFactor;
		}
		else
		{
			PowerFraction = FMath::Max(MinShotPower, PowerFraction / ShankPowerFactor);
		}

		const auto Accuracy = (FMath::RandBool() ? 1 : -1) * AIConfigEntry->ShankAccuracy;

		UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Shank Shot - PowerFraction=%.2f; ShankPowerFactor=%.2f; ShankAccuracy=%.2f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), PowerFraction, ShankPowerFactor, Accuracy);

		return FShotErrorResult
		{
			.PowerFraction = PowerFraction,
			.Accuracy = Accuracy
		};
	}

	const auto AccuracyDeviation = AIConfigEntry->DefaultAccuracyDelta;
	const auto PowerDeviation = AIConfigEntry->DefaultPowerDelta;

	const auto Accuracy = GenerateAccuracy(AccuracyDeviation);
	PowerFraction = GeneratePowerFraction(PowerFraction, PowerDeviation);

	// use default calculation
	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s-%s: CalculateShotError - Default - PowerFraction=%.2f; Accuracy = %.2f; AccuracyDeviation=%.2f; PowerDeviation=%.2f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), PowerFraction, Accuracy, AccuracyDeviation, PowerDeviation);

	return FShotErrorResult
	{
		.PowerFraction = PowerFraction,
		.Accuracy = Accuracy,
	};
}

FString FAIShotContext::ToString() const
{
	return FString::Printf(TEXT("PlayerPawn=%s; PlayerState=%s; ShotType=%s"),
		*LoggingUtils::GetName(PlayerPawn), *LoggingUtils::GetName(PlayerState), *LoggingUtils::GetName(ShotType));
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
}
