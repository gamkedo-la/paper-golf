// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#include "Utils/VisualLoggerUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VisualLoggerUtils)

#if ENABLE_VISUAL_LOG
#include "PhysicsEngine/BodySetup.h"

#include "Components/ShapeComponent.h"
#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Debug/PGConsoleVars.h"
#include "PGConstants.h"

#include <atomic>

namespace
{
	using ContextPtr = TWeakObjectPtr<const UObject>;

	// Fine for these to be file scoped variables as only accessed on game thread
	bool bStartedAutomaticVisualLoggerRecording = false;
	bool bVisualLoggerStartedByUs = false;
	ContextPtr RecordingContext = nullptr;

	void DrawBodySetup(FVisualLogEntry& Snapshot, const FName& CategoryName, const UPrimitiveComponent& Component, const UBodySetup& BodySetup, const FColor& Color, bool bUseWires);
}

void PG::VisualLoggerUtils::DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color, bool bUseWires)
{
	const auto Mesh = Component.GetStaticMesh();
	if (!IsValid(Mesh) || !Mesh->GetBodySetup())
	{
		return;
	}

	const auto BodySetup = Mesh->GetBodySetup();
	DrawBodySetup(Snapshot, CategoryName, Component, *BodySetup, Color, bUseWires);
}

void PG::VisualLoggerUtils::DrawPrimitiveComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UPrimitiveComponent& Component, const FColor& Color, bool bUseWires)
{
	const auto BodyInstance = Component.GetBodyInstance();
	if (!BodyInstance || !BodyInstance->GetBodySetup())
	{
		return;
	}

	const auto BodySetup = BodyInstance->GetBodySetup();
	DrawBodySetup(Snapshot, CategoryName, Component, *BodySetup, Color, bUseWires);
}

void PG::VisualLoggerUtils::StartAutomaticRecording(const UObject* Context)
{
#if PG_DEBUG_ENABLED

	check(Context);
	check(IsInGameThread());

	const bool bStartRecord = CAutomaticVisualLoggerRecording.GetValueOnGameThread();
	if (!bStartRecord)
	{
		UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("StartAutomaticRecording - ignored in %s as console variable is false"),
			*LoggingUtils::GetName(Context));
		return;
	}

	if (RecordingContext.IsExplicitlyNull())
	{
		bStartedAutomaticVisualLoggerRecording = true;
		RecordingContext = Context;

		auto& VisualLogger = FVisualLogger::Get();
		if (!VisualLogger.IsRecording()) {
			bVisualLoggerStartedByUs = true;
		}
		VisualLogger.SetIsRecordingToFile(true);
	}
	else if(auto ExistingContext = RecordingContext.Get(); ExistingContext)
	{
		UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("StartAutomaticRecording - ignored in %s as already started by %s"),
			*LoggingUtils::GetName(Context), *ExistingContext->GetName());
	}
	else
	{
		UE_VLOG_UELOG(Context, LogPGCore, Warning, TEXT("StartAutomaticRecording - Ignoring Context=%s as previous recording was never stopped"),
			*LoggingUtils::GetName(Context));
	}

#endif
}

void PG::VisualLoggerUtils::RecheckAutomaticRecording(const UObject* Context)
{
#if PG_DEBUG_ENABLED

	check(Context);
	check(IsInGameThread());

	const bool bShouldBeRecording = CAutomaticVisualLoggerRecording.GetValueOnGameThread();
	if (!bShouldBeRecording && bStartedAutomaticVisualLoggerRecording)
	{
		const auto ExistingContext = RecordingContext.Get();

		if (Context == ExistingContext)
		{
			UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("RecheckAutomaticRecording - autorecord changed to false - Context=%s"),
				*LoggingUtils::GetName(Context));

			StopAutomaticRecording(Context);
		}
	}
#endif
}

void PG::VisualLoggerUtils::StopAutomaticRecording(const UObject* Context)
{
#if PG_DEBUG_ENABLED

	check(Context);
	check(IsInGameThread());

	const bool bHasStarted = bStartedAutomaticVisualLoggerRecording;

	if (!bHasStarted)
	{
		UE_VLOG_UELOG(Context, LogPGCore, Warning, TEXT("StopAutomaticRecording - Ignoring Context=%s as recording was never started"),
			*LoggingUtils::GetName(Context));
		return;
	}

	const auto ExistingContext = RecordingContext.Get();
	const bool bExistingContextIsValid = RecordingContext.IsValid();

	if(Context == ExistingContext || !bExistingContextIsValid)
	{
		if (!bExistingContextIsValid)
		{
			UE_VLOG_UELOG(Context, LogPGCore, Warning, TEXT("StopAutomaticRecording - Stopping by Context=%s but previous start was not properly stopped"),
				*LoggingUtils::GetName(Context));
		}

		auto& VisualLogger = FVisualLogger::Get();
		VisualLogger.SetIsRecordingToFile(false);

		if (bVisualLoggerStartedByUs) {
			VisualLogger.SetIsRecording(false);
		}

		bStartedAutomaticVisualLoggerRecording = false;
		RecordingContext = nullptr;
	}
	else
	{
		UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("StopAutomaticRecording - Ignoring Context=%s as was started by %s"),
			*LoggingUtils::GetName(Context), *LoggingUtils::GetName(ExistingContext));
	}
#endif
}

bool UVisualLoggerUtils::IsRecording() { return FVisualLogger::Get().IsRecording(); }
void UVisualLoggerUtils::StartRecording()
{
	if (IsRecording())
	{
		return;
	}

	// Equivalent to console command "vislog record"
	if (GIsEditor)
	{
		FVisualLogger::Get().SetIsRecording(true);
	}
	else
	{
		FVisualLogger::Get().SetIsRecordingToFile(true);
	}
}
void UVisualLoggerUtils::StopRecording()
{
	if (!IsRecording())
	{
		return;
	}

	// Equivalent to console command "vislog stop"
	if (GIsEditor)
	{
		FVisualLogger::Get().SetIsRecording(false);
	}
	else
	{
		FVisualLogger::Get().SetIsRecordingToFile(false);
	}

#if PG_DEBUG_ENABLED
	// Clear out any automatic recording context
	bStartedAutomaticVisualLoggerRecording = false;
	RecordingContext = nullptr;
#endif
}

namespace
{
	void DrawBodySetup(FVisualLogEntry& Snapshot, const FName& CategoryName, const UPrimitiveComponent& Component, const UBodySetup& BodySetup, const FColor& Color, bool bUseWires)
	{
		const auto& AggGeom = BodySetup.AggGeom;

		const auto& Transform = Component.GetComponentTransform();

		// Render all the collision shapes even if multiple types are combined
		if (!AggGeom.ConvexElems.IsEmpty())
		{
			for (const auto& ConvexElm : AggGeom.ConvexElems)
			{
				// Make a copy as we need to transform the position from model to world space
				auto VertexData = ConvexElm.VertexData;
				for (auto& Vertex : VertexData)
				{
					Vertex = Transform.TransformPosition(Vertex);
				}

				Snapshot.AddMesh(
					VertexData,
					ConvexElm.IndexData,
					CategoryName,
					ELogVerbosity::Log,
					Color);
			}
		}
		if (!AggGeom.BoxElems.IsEmpty())
		{
			for (const auto& BoxElem : AggGeom.BoxElems)
			{
				Snapshot.AddBox(
					FBox::BuildAABB(FVector::ZeroVector, FVector{ BoxElem.X, BoxElem.Y, BoxElem.Z }),
					// BoxElem transform includes the center
					(BoxElem.GetTransform() * Transform).ToMatrixWithScale(),
					CategoryName,
					ELogVerbosity::Log,
					Color, {}, {},
					bUseWires
				);
			}
		}
		if (!AggGeom.SphereElems.IsEmpty())
		{
			for (const auto& SphereElem : AggGeom.SphereElems)
			{
				Snapshot.AddSphere(
					Transform.TransformPosition(SphereElem.Center),
					SphereElem.Radius,
					CategoryName,
					ELogVerbosity::Log,
					Color, {},
					bUseWires
				);
			}
		}
		if (!AggGeom.SphylElems.IsEmpty())
		{
			for (const auto& SphylElm : AggGeom.SphylElems)
			{
				// Half height expects the radius to be added on as well - determined by visually aligning to the primitive rendering in the editor
				const auto HalfHeight = SphylElm.Length * 0.5f + SphylElm.Radius;
				// Capsule expects position at base and not the center like the transform

				Snapshot.AddCapsule(
					Transform.TransformPosition(SphylElm.Center - FVector::ZAxisVector * HalfHeight),
					HalfHeight,
					SphylElm.Radius,
					Transform.GetRotation() * SphylElm.Rotation.Quaternion(),
					CategoryName,
					ELogVerbosity::Log,
					Color, {},
					bUseWires
				);
			}
		}
		if (!AggGeom.TaperedCapsuleElems.IsEmpty())
		{
			for (const auto& CapsuleElm : AggGeom.TaperedCapsuleElems)
			{
				const auto RadiiSum = CapsuleElm.Radius0 + CapsuleElm.Radius1;
				const auto Radius = RadiiSum * 0.5f;
				const auto HalfHeight = CapsuleElm.Length * 0.5f + Radius;

				Snapshot.AddCapsule(
					Transform.TransformPosition(CapsuleElm.Center + FVector::ZAxisVector * HalfHeight),
					HalfHeight,
					Radius,
					Transform.GetRotation() * CapsuleElm.Rotation.Quaternion(),
					CategoryName,
					ELogVerbosity::Log,
					Color, {},
					bUseWires
				);
			}
		}
	}
}

#endif
