// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/CollisionDampeningComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "PGPawnLogging.h"

#include "Components/StaticMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CollisionDampeningComponent)

UCollisionDampeningComponent::UCollisionDampeningComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCollisionDampeningComponent::OnFlick()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: OnFlick"), *LoggingUtils::GetName(GetOwner()), *GetName());

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	ResetState();
	FlickTime = GetWorld()->GetTimeSeconds();
}

void UCollisionDampeningComponent::OnShotFinished()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: OnShotFinished"), *LoggingUtils::GetName(GetOwner()), *GetName());

	DisableCollisionDampening();
}

void UCollisionDampeningComponent::BeginPlay()
{
	Super::BeginPlay();

	if (OwnerStaticMeshComponent)
	{
		InitInitialDampeningValues();
		DisableCollisionDampening();
	}
}

void UCollisionDampeningComponent::RegisterCollisions()
{
	if (!ShouldEnableCollisionDampening())
	{
		return;
	}

	if (!ValidateConfig())
	{
		return;
	}

	auto MyOwner = GetOwner();
	if (!ensureMsgf(MyOwner, TEXT("%s: Owner was NULL"), *GetName()))
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: RegisterCollisions"), *LoggingUtils::GetName(GetOwner()), *GetName());

	OwnerStaticMeshComponent = MyOwner->GetComponentByClass<UStaticMeshComponent>();

	if (OwnerStaticMeshComponent)
	{
		RegisterComponent(OwnerStaticMeshComponent);
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Warning,
			TEXT("%s-%s: RegisterCollisions - Could not find UStaticMeshComponent to register for hit callback"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}
}

void UCollisionDampeningComponent::OnNotifyRelevantCollision(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& NormalImpulse)
{
	// Disabled until flick
	if (!IsCollisionDampeningActive())
	{
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: OnNotifyRelevantCollision - HitComponent=%s; Hit=%s; NormalImpulse=%s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(HitComponent), *Hit.ToString(), *NormalImpulse.ToCompactString());

	if (!HitComponent)
	{
		return;
	}

	auto World = GetWorld();
	if (!ensure(World))
	{
		return;
	}

	const auto TimeSeconds = World->GetTimeSeconds();

	if (TimeSeconds - FlickTime <= FlickTimeDelay)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: OnNotifyRelevantCollision - SKIP - FlickTime=%f; TimeSeconds=%f <= FlickTimeDelay=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), FlickTime, TimeSeconds, FlickTimeDelay);
		return;
	}

	if (HitComponent == LastHitComponent && TimeSeconds - LastHitTime <= CollisionTimeRecordDelay)
	{
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, VeryVerbose, TEXT("%s-%s: OnNotifyRelevantCollision - SKIP - LastHitTime=%f; TimeSeconds=%f <= CollisionTimeRecordDelay=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(), LastHitTime, TimeSeconds, CollisionTimeRecordDelay);
		return;
	}

	++HitCount;
	LastHitTime = TimeSeconds;
	LastHitComponent = HitComponent;

	UpdateDampeningValues();
}

void UCollisionDampeningComponent::ResetState()
{
	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: ResetState"), *LoggingUtils::GetName(GetOwner()), *GetName());

	DisableCollisionDampening();

	HitCount = 0;
	LastHitComponent = nullptr;
	LastHitTime = 0.0f;

	if (OwnerStaticMeshComponent)
	{
		OwnerStaticMeshComponent->SetAngularDamping(InitialAngularDampening);
		OwnerStaticMeshComponent->SetLinearDamping(InitialLinearDampening);
	}
}

bool UCollisionDampeningComponent::ValidateConfig() const
{
	bool bValid = true;

	if (!ensureMsgf(AngularDampeningCurve, TEXT("AngularDampeningCurve is NULL")))
	{
		bValid = false;
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: ValidateConfig - AngularDampeningCurve was NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	if (!ensureMsgf(LinearDampeningCurve, TEXT("LinearDampeningCurve is NULL")))
	{
		bValid = false;
		UE_VLOG_UELOG(GetOwner(), LogPGPawn, Error, TEXT("%s-%s: ValidateConfig - LinearDampeningCurve was NULL"), *LoggingUtils::GetName(GetOwner()), *GetName());
	}

	return bValid;
}

void UCollisionDampeningComponent::InitInitialDampeningValues()
{
	check(OwnerStaticMeshComponent);

	InitialAngularDampening = OwnerStaticMeshComponent->GetAngularDamping();
	InitialLinearDampening = OwnerStaticMeshComponent->GetLinearDamping();

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: InitInitialDampeningValues: InitialAngularDampening=%f; InitialLinearDampening=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), InitialAngularDampening, InitialLinearDampening);
}

void UCollisionDampeningComponent::UpdateDampeningValues()
{
	const auto NewAngularDampening = GetDampeningValue(InitialAngularDampening, AngularDampeningCurve);
	const auto NewLinearDampening = bEnableLinearDampening ? GetDampeningValue(InitialLinearDampening, LinearDampeningCurve) : InitialLinearDampening;

	check(OwnerStaticMeshComponent);

	UE_VLOG_UELOG(GetOwner(), LogPGPawn, Log, TEXT("%s-%s: UpdateDampeningValues: Angular: %f -> %f; Linear: %f -> %f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), OwnerStaticMeshComponent->GetAngularDamping(), NewAngularDampening,
		OwnerStaticMeshComponent->GetLinearDamping(), NewLinearDampening);

	OwnerStaticMeshComponent->SetAngularDamping(NewAngularDampening);
	OwnerStaticMeshComponent->SetLinearDamping(NewLinearDampening);
}

float UCollisionDampeningComponent::GetDampeningValue(float InitialValue, const UCurveFloat* Curve) const
{
	check(Curve);

	return InitialValue * Curve->GetFloatValue(HitCount);
}

void UCollisionDampeningComponent::DisableCollisionDampening()
{
	FlickTime = -1.0f;
}

bool UCollisionDampeningComponent::IsCollisionDampeningActive() const
{
	return FlickTime >= 0;
}

bool UCollisionDampeningComponent::ShouldEnableCollisionDampening() const
{
	// only register on server
	return bEnableCollisionDampening && GetOwner() && GetOwner()->HasAuthority();
}
