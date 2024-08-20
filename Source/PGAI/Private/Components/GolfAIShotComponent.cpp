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

#include <array>
#include <iterator>

#include UE_INLINE_GENERATED_CPP_BY_NAME(GolfAIShotComponent)

namespace
{
	constexpr const std::array ShotPitchAngles = { 45.0f, 60.0f, 75.0f, 30.0f, 0.0f, -15.0f };
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
}

TOptional<UGolfAIShotComponent::FAIShotSetupResult> UGolfAIShotComponent::CalculateShotParams() const
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
	const auto& FocusActorLocation = FocusActor->GetActorLocation();

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

	// Add error to power calculation and accuracy
	FlickParams.PowerFraction = GeneratePowerFraction(PowerFraction);
	FlickParams.Accuracy = GenerateAccuracy();

	const auto ShotPitch = CalculateShotPitch(FlickLocation, FlickMaxSpeed, FlickParams.PowerFraction);

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateShotParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f; ShotPitch=%.1f; FlickDragForceMultiplier=%1.f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset, ShotPitch, FlickDragForceMultiplier);

	return FAIShotSetupResult
	{
		.FlickParams = FlickParams,
		.ShotPitch = ShotPitch
	};
}

float UGolfAIShotComponent::GenerateAccuracy() const
{
	return FMath::FRandRange(-AccuracyDeviation, AccuracyDeviation);
}

float UGolfAIShotComponent::GeneratePowerFraction(float InPowerFraction) const
{
	return FMath::Clamp(InPowerFraction * (1 + FMath::RandRange(-PowerDeviation, PowerDeviation)), MinShotPower, 1.0f);
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

float UGolfAIShotComponent::CalculateShotPitch(const FVector& FlickLocation, float FlickMaxSpeed, float PowerFraction) const
{
	static_assert(ShotPitchAngles.size() > 0, "ShotPitchAngles must have at least one element");

	if constexpr(ShotPitchAngles.size() == 1)
	{
		return ShotPitchAngles.front();
	}
	else
	{
		const auto PlayerPawn = ShotContext.PlayerPawn;
		check(PlayerPawn);

		auto World = GetWorld();
		check(World);

		// Via the work energy principle, reducing the power by N will reduce the speed by factor of sqrt(N)
		const auto FlickSpeed = FlickMaxSpeed * FMath::Sqrt(PowerFraction);
		const auto FlickDirection = PlayerPawn->GetFlickDirection();

		for (auto It = ShotPitchAngles.begin(), End = std::next(ShotPitchAngles.begin(), ShotPitchAngles.size() - 1); It != End; ++It)
		{
			const auto PitchAngle = *It;

			bool bPass = TraceShotAngle(*PlayerPawn, FlickLocation, FlickDirection, FlickSpeed, PitchAngle);
			if (bPass)
			{
				return PitchAngle;
			}
		}

		// return default angle, which is last element
		return ShotPitchAngles.back();
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
		.Accuracy = GenerateAccuracy()
	};

	UE_VLOG_UELOG(this, LogPGAI, Log, TEXT("%s: CalculateShotParams - ShotType=%s; Power=%.2f; Accuracy=%.2f; ZOffset=%.1f"),
		*GetName(), *LoggingUtils::GetName(FlickParams.ShotType), FlickParams.PowerFraction, FlickParams.Accuracy, FlickParams.LocalZOffset);

	return FAIShotSetupResult
	{
		.FlickParams = FlickParams,
		.ShotPitch = 45.0f
	};
}

FString FAIShotContext::ToString() const
{
	return FString::Printf(TEXT("PlayerPawn=%s; PlayerState=%s; ShotType=%s"),
		*LoggingUtils::GetName(PlayerPawn), *LoggingUtils::GetName(PlayerState), *LoggingUtils::GetName(ShotType));
}
