// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "Components/MovingObstacle.h"
#include "Components/AudioComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Utils/VisualLoggerUtils.h"
#include "Logging/LoggingUtils.h"
#include "PGGameplayLogging.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovingObstacle)

AMovingObstacle::AMovingObstacle()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	if (RootComponent)
	{
		SetRootComponent(RootComponent);
	}

	movementPath = CreateDefaultSubobject<USplineComponent>(TEXT("Path"));
	if (movementPath)
	{
		movementPath->SetupAttachment(RootComponent);
	}

	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Obstacle"));
	if (staticMesh)
	{
		staticMesh->SetupAttachment(RootComponent);
	}

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	AudioComponent->SetupAttachment(staticMesh);
	AudioComponent->SetAutoActivate(false);
}

void AMovingObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

// Called when the game starts or when spawned
void AMovingObstacle::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AMovingObstacle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: EndPlay - %s"), *GetName(), *LoggingUtils::GetName(EndPlayReason));

	Super::EndPlay(EndPlayReason);

#if ENABLE_VISUAL_LOG
	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);
#endif
}

void AMovingObstacle::EnableTick(bool bEnabled)
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: EnableTick - bEnabled=%s"), *GetName(), LoggingUtils::GetBoolString(bEnabled));

	PrimaryActorTick.SetTickFunctionEnable(bEnabled);
}

void AMovingObstacle::Init()
{
	UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: Init"), *GetName());

	// Only tick on the server as we will replicate the movement down to clients
	EnableTick(HasAuthority());

	check(staticMesh);
	staticMesh->SetIsReplicated(true);

	// Only enable collision on server and let the collision response replicate to clients
	if (HasAuthority())
	{
		const auto& RelativeTransform = staticMesh->GetRelativeTransform();
		InitialRelativeLocation = RelativeTransform.GetLocation();
		InitialRelativeRotation = RelativeTransform.GetRotation();
	}
	else
	{
		staticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	PlayAudioIfValid();

	// regularly draw updates if visual logging is enabled so we can see the obstacle in the visual logger
#if ENABLE_VISUAL_LOG
	GetWorldTimerManager().SetTimer(VisualLoggerTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UE_VLOG(this, LogPGGameplay, Log, TEXT("Get Moving Obstacle State"));
	}), 0.05f, true);
#endif
}

void AMovingObstacle::PlayAudioIfValid()
{
	if (AudioComponent && AudioComponent->Sound)
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PlayAudioIfValid - Playing sound: %s"), *GetName(), *LoggingUtils::GetName(AudioComponent->Sound));
		AudioComponent->Play();
	}
	else
	{
		UE_VLOG_UELOG(this, LogPGGameplay, Log, TEXT("%s: PlayAudioIfValid - No sound to play"), *GetName());

	}
}

void AMovingObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetDistance();
	EvaluatePosition();
}

void AMovingObstacle::EvaluatePosition()
{
	const auto AdjustedDistance = bReverseDirection ? movementPath->GetSplineLength() - distance : distance;
	const auto& SplineTransform = movementPath->GetTransformAtDistanceAlongSpline(AdjustedDistance, ESplineCoordinateSpace::World);
	// Preserve local transform of static mesh
	const auto& LocalTransform = staticMesh->GetRelativeTransform();

	// Combine the local transform with the spline transform
	FTransform UpdatedTransform = staticMesh->GetComponentTransform();
	UpdatedTransform.SetLocation(SplineTransform.GetLocation() + InitialRelativeLocation);
	UpdatedTransform.SetRotation(InitialRelativeRotation * SplineTransform.GetRotation());

	// Set the final world transform of the static mesh
	staticMesh->SetWorldTransform(UpdatedTransform);
}

void AMovingObstacle::SetDistance()
{
	if (distance >= movementPath->GetSplineLength())
	{
		if (!movementPath->IsClosedLoop())
		{
			valueToAdd = -1;
		}
		else
		{
			distance = 0;
		}
	}

	if (distance <= 0)
	{
		valueToAdd = 1;
	}

	distance+=valueToAdd*speed;
}

#pragma region Visual Logger

#if ENABLE_VISUAL_LOG
void AMovingObstacle::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = FString::Printf(TEXT("MovingObstacle (%s)"), *GetName());

	Category.Add("Distance", FString::Printf(TEXT("%.1f"), distance));
	Category.Add("Speed", FString::Printf(TEXT("%.1f"), speed));
	Category.Add("ReverseDirection", LoggingUtils::GetBoolString(bReverseDirection));

	if (staticMesh)
	{
		PG::VisualLoggerUtils::DrawStaticMeshComponent(*Snapshot, LogPGGameplay.GetCategoryName(), *staticMesh, FColor::Orange);
	}

	Snapshot->Status.Add(Category);
}
#endif

#pragma endregion Visual Logger