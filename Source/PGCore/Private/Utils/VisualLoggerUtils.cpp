// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#if ENABLE_VISUAL_LOG

#include "Utils/VisualLoggerUtils.h"

#include "PhysicsEngine/BodySetup.h"
#include "Utils/CollisionUtils.h"

#include "Logging/LoggingUtils.h"
#include "PGCoreLogging.h"
#include "VisualLogger/VisualLogger.h"

#include "Debug/PGConsoleVars.h"
#include "PGConstants.h"

#include <atomic>

namespace
{
	std::atomic_bool bStartedAutomaticVisualLoggerRecording(false);

	using ContextPtr = TWeakObjectPtr<const UObject>;
	std::atomic<ContextPtr> RecordingContext(nullptr);
}

void PG::VisualLoggerUtils::DrawStaticMeshComponent(FVisualLogEntry& Snapshot, const FName& CategoryName, const UStaticMeshComponent& Component, const FColor& Color)
{
	const auto Mesh = Component.GetStaticMesh();
	if (!IsValid(Mesh) || !Mesh->GetBodySetup())
	{
		return;
	}

	const auto BodySetup = Mesh->GetBodySetup();
	const auto& AggGeom = BodySetup->AggGeom;

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
				Color
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
				Color
			);
		}
	}
	if (!AggGeom.SphylElems.IsEmpty())
	{
		for (const auto& SphylElm : AggGeom.SphylElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(SphylElm.Center),
				SphylElm.Length * 0.5f,
				SphylElm.Radius,
				Transform.GetRotation() * SphylElm.Rotation.Quaternion(),
				CategoryName,
				ELogVerbosity::Log,
				Color);
		}
	}
	if (!AggGeom.TaperedCapsuleElems.IsEmpty())
	{
		for (const auto& CapsuleElm : AggGeom.TaperedCapsuleElems)
		{
			Snapshot.AddCapsule(
				Transform.TransformPosition(CapsuleElm.Center),
				(CapsuleElm.Radius0 + CapsuleElm.Radius1) * 0.5f,
				CapsuleElm.Length * 0.5f,
				Transform.GetRotation() * CapsuleElm.Rotation.Quaternion(),
				CategoryName,
				ELogVerbosity::Log,
				Color);
		}
	}
}

void PG::VisualLoggerUtils::StartAutomaticRecording(const UObject* Context)
{
#if PG_DEBUG_ENABLED

	check(Context);

	const bool bStartRecord = CAutomaticVisualLoggerRecording.GetValueOnGameThread();
	ContextPtr ExistingContext{};

	if (bStartRecord && RecordingContext.compare_exchange_strong(ExistingContext, Context))
	{
		bStartedAutomaticVisualLoggerRecording = true;
		FVisualLogger::Get().SetIsRecordingToFile(true);
	}
	else if(bStartRecord && ExistingContext.IsValid())
	{
		UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("StartAutomaticRecording - ignored in %s as already started by %s"),
			*LoggingUtils::GetName(Context), *LoggingUtils::GetName(ExistingContext.Get()));
	}

#endif
}

void PG::VisualLoggerUtils::StopAutomaticRecording(const UObject* Context)
{
#if PG_DEBUG_ENABLED

	check(Context);

	const bool bHasStarted = bStartedAutomaticVisualLoggerRecording;
	const UObject* ExistingContext = RecordingContext.load().Get();

	// benign race condition here
	if(bHasStarted && Context == ExistingContext)
	{
		FVisualLogger::Get().SetIsRecordingToFile(false);
		bStartedAutomaticVisualLoggerRecording = false;
		RecordingContext = nullptr;
	}
	else if (bHasStarted && ExistingContext)
	{
		UE_VLOG_UELOG(Context, LogPGCore, Log, TEXT("StopAutomaticRecording - ignored in %s as was started by %s"),
			*LoggingUtils::GetName(Context), *ExistingContext->GetName());
	}
#endif
}
#endif
