// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Obstacles/OscillatingFan.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Utils/VisualLoggerUtils.h"
#include "PGGameplayLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OscillatingFan)

AOscillatingFan::AOscillatingFan()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
}

void AOscillatingFan::BeginPlay()
{
	Super::BeginPlay();

	// TODO: We should subscribe to game events to disable the force if an end overlap event doesn't happen before the end of the player's turn
	// or maybe even the end of the hole

	// regularly draw updates if visual logging is enabled so we can see the obstacle in the visual logger
#if ENABLE_VISUAL_LOG
	GetWorldTimerManager().SetTimer(VisualLoggerTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UE_VLOG(this, LogPGGameplay, Log, TEXT("Get Oscillating Fan State"));
		}), 0.05f, true);
#endif
}

void AOscillatingFan::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

#if ENABLE_VISUAL_LOG
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
#endif
}

void AOscillatingFan::SetInfluenceCollider(UPrimitiveComponent* InCollider)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetInfluenceCollider - %s"),
		*GetName(), *LoggingUtils::GetName(InCollider));

	if (!ensureMsgf(InCollider, TEXT("%s: SetCollider - Collider is null"), *GetName()))
	{
		return;
	}

	InfluenceCollider = InCollider;

	InfluenceCollider->OnComponentBeginOverlap.AddUniqueDynamic(this, &AOscillatingFan::OnComponentBeginOverlap);
	InfluenceCollider->OnComponentEndOverlap.AddUniqueDynamic(this, &AOscillatingFan::OnComponentEndOverlap);
}

void AOscillatingFan::OnComponentBeginOverlap(UPrimitiveComponent* InOverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentOverlapBegin - %s-%s"),
		*GetName(), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComp));
	
	// Make sure overlap with a static mesh component and not some other primitive component
	auto StaticMeshComponent = Cast<UStaticMeshComponent>(OtherComp);
	if(!StaticMeshComponent)
	{
		return;
	}
	
	OverlappedComponent = StaticMeshComponent;
	SetForceActive(true);
}

void AOscillatingFan::OnComponentEndOverlap(UPrimitiveComponent* InOverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: OnComponentOverlapEnd - %s-%s"),
		*GetName(), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComp));

	if(OverlappedComponent == OtherComp)
	{
		OverlappedComponent = nullptr;
		SetForceActive(false);
	}
}

void AOscillatingFan::SetForceActive(bool bActive)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: SetForceActive - %s"), *GetName(), LoggingUtils::GetBoolString(bActive));
	
	PrimaryActorTick.SetTickFunctionEnable(bActive);
}

bool AOscillatingFan::ShouldApplyForceTo(const UPrimitiveComponent& Component, const FAirflowData& AirflowData) const
{
	if(!Component.IsSimulatingPhysics())
	{
		return false;
	}

	// make sure facing toward the airflow direction
	const auto AirflowOriginToComponent = Component.GetComponentLocation() - AirflowData.Origin;
	const auto AirflowAlignment = AirflowOriginToComponent | AirflowData.Direction;

	if(AirflowAlignment <= 0)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: ShouldApplyForceTo - %s-%s - Not facing airflow direction"),
			*GetName(), *Component.GetName(), *LoggingUtils::GetName(Component.GetOwner()));
		return false;
	}

	const auto DistSq = AirflowOriginToComponent.SizeSquared();

	// If outside the radial falloff then don't apply force
	const auto RadialFalloffSq = FMath::Square(ForceRadialFalloffDistance + MaxForceDistance);

	if (DistSq > RadialFalloffSq)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: ShouldApplyForceTo - %s-%s - Outside radial falloff: Dist=%fm; RadialFalloffDist=%fm"),
			*GetName(), *Component.GetName(), *LoggingUtils::GetName(Component.GetOwner()), FMath::Sqrt(DistSq) / 100, FMath::Sqrt(RadialFalloffSq) / 100);
		return false;
	}

	return true;
}

FVector AOscillatingFan::CalculateAirflowForce(const UPrimitiveComponent& Component, const FAirflowData& AirFlowData) const
{
	// calculate distance to origin
	const auto Dist = FVector::Dist(Component.GetComponentLocation(), AirFlowData.Origin);

	float ForceMagnitude;
	if(Dist <= MaxForceDistance)
	{
		ForceMagnitude = MaxForceStrength;
	}
	else
	{
		// Take excess distance beyond max force distance
		const auto ForceReduceDist = Dist - MaxForceDistance;

		const auto ForceRadialFalloffDistanceFactor = FMath::Min(1.0,
			FMath::Square(1 - (Dist - MaxForceDistance) / (ForceRadialFalloffDistance - MaxForceDistance)));
		// use inverse square law to adjust force
		ForceMagnitude = ForceRadialFalloffDistanceFactor * MaxForceStrength;
	}
	
	UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose,
		TEXT("%s: CalculateAirflowForce - ForceMagnitude=%f; Origin=%s; Direction=%s; Dist=%fm; ThresholdDist=%fm;"), 
		*GetName(), ForceMagnitude, *AirFlowData.Origin.ToCompactString(), *AirFlowData.Direction.ToCompactString(),
		Dist / 100, MaxForceDistance / 100);
		
	return AirFlowData.Direction * ForceMagnitude;
}

void AOscillatingFan::ApplyAirflowForce(UPrimitiveComponent& Component, const FVector& Force) const
{
	UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: ApplyAirflowForce - %s to %s-%s"),
		*GetName(), *Force.ToCompactString(),
		*LoggingUtils::GetName(Component.GetOwner()),
		*Component.GetName());

	Component.AddForce(Force);
}

void AOscillatingFan::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Should only be called when force is active
	if(!IsValid(OverlappedComponent))
	{
		UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: Tick - OverlappedComponent is null"), *GetName());
		SetForceActive(false);
		return;
	}
	
	FAirflowData AirflowData;
	GetAirflowOriginAndDirection(AirflowData.Origin, AirflowData.Direction);

	if(!ShouldApplyForceTo(*OverlappedComponent, AirflowData))
	{
		UE_VLOG_UELOG(this, LogPGGameplay, VeryVerbose, TEXT("%s: Tick - Should not apply force to %s-%s"),
			*GetName(), *OverlappedComponent->GetName(), *LoggingUtils::GetName(OverlappedComponent->GetOwner()));
		return;
	}

	const auto ForceToApply = CalculateAirflowForce(*OverlappedComponent, AirflowData);
	ApplyAirflowForce(*OverlappedComponent, ForceToApply);

	UE_VLOG_ARROW(this, LogPGGameplay, Verbose, 
		AirflowData.Origin,
		AirflowData.Origin + ForceToApply.GetSafeNormal() * FVector::Dist(AirflowData.Origin, OverlappedComponent->GetComponentLocation()),
		FColor::Orange, TEXT("Airflow Force: %.1f%%"), ForceToApply.Size() / MaxForceStrength * 100
	);
}

void AOscillatingFan::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

#pragma region VisualLogger

#if ENABLE_VISUAL_LOG
void AOscillatingFan::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("OscillatingFan (%s)"), *GetName());

	Category.Add("OverlappedActor", OverlappedComponent ? *LoggingUtils::GetName(OverlappedComponent->GetOwner()) : TEXT("None"));

	if (auto FanHeadMesh = GetFanHeadMesh())
	{
		PG::VisualLoggerUtils::DrawStaticMeshComponent(*Snapshot, LogPGGameplay.GetCategoryName(), *FanHeadMesh, FColor::Orange);
	}

	if (InfluenceCollider)
	{
		PG::VisualLoggerUtils::DrawPrimitiveComponent(*Snapshot, LogPGGameplay.GetCategoryName(), *InfluenceCollider, OverlappedComponent ? FColor::Green : FColor::Red, true);
	}
}

#endif

#pragma endregion VisualLogger
